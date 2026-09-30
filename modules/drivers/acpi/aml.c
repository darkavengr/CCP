#include <stdint.h>
#include <stddef.h>
#include "vfs.h"
#include "string.h"
#include "acpi.h"
#include "aml.h"

ACPITreeNode *ACPIRoot=NULL;
ACPITreeNode *ACPICurrentScope=NULL;
ACPITreeNode *ACPIParentScope=NULL;
char *InvalidNestedTerm="acpi: Invalid nested term %s in %s block\n";
char *ACPITermOutsideBlock="acpi: %s term outside of DEVICE or SCOPE block\n";
char *ACPITerms[] = { "Device","Scope","Processor","ThermalZone","PowerResource","Method" };
char *AMLMethodExecError="acpi: %s Executing AML method %s\n";
AMLMETHODCALLSTACK *AMLMethodCallStack=NULL;
AMLMETHODCALLSTACK *AMLMethodCallStack_end=NULL;

/*
* Get root node
*
* In: Nothing
*
* Returns: Pointer to root node
*
*/

ACPITreeNode *GetACPIRootNode(void) {
return(ACPIRoot);
}

/*
* Recursively find ACPI node
*
* In: name	Node name
      node	Pointer to start node
*
* Returns: Pointer to node or NULL on error
*
*/
ACPITreeNode *FindACPINode(char *name) {
size_t NodeCount=0;
char *tokens[MAX_PATH][MAX_PATH];
int tokencount;
int tc;
ACPITreeNode *node;

if((char) *name == '\\') {	/* starting from root */
	node=ACPIRoot;
}
else
{
	node=ACPICurrentScope;	/* starting from current scope */
}

tc=tokenize_line(name,tokens,"\\");	/* tokenize line */

/* search through nodes */

for(tokencount=0;tokencount < tc;tokencount++) { 
	for(NodeCount=0;NodeCount < node->NumberOfSubNodes;NodeCount++) {
		if(strncmp(node->subnode[NodeCount]->name,tokens[tokencount]) == 0) {

			node=node->subnode[NodeCount];
			if(node != NULL) return(NULL);
		}
}
			
return(NULL);
}

/*
* Get AML field length
*
* In: amlbuf	Pointer to AML data
*
*  Returns: Field length
*
*/
size_t AMLGetFieldLength(uint8_t *amlbuf) {
uint8_t NumberOfLengthBytes;
uint8_t amlbyte;
uint32_t FieldLength;
size_t ShiftCount=6;

amlbyte=(uint8_t) *amlbuf++;		/* get first byte */

NumberOfLengthBytes=(amlbyte & 0xC0) >> 6;	/* Get number of length bytes */
FieldLength=amlbyte & 0xC0;

/* add additional bytes to length */

do {
	FieldLength += (uint32_t) (*amlbuf << ShiftCount);
	ShiftCount += 8;
while(NumberOfLengthBytes-- > 0);

return(FieldLength);
}

/*
* Get AML string
*
* In: amlbuf	Pointer to AML data
      buf	Buffer

*  Returns: Updated pointer to AML data
*
*/
char *GetAMLString(uint8_t *amlbuf,char *buf) {
char *bufptr=buf;
uint8_t *amlptr=amlbuf;
uint8_t NumberOfSegments;

if((uint8_t) *amlptr++ == 0) {		/* null */
	*bufptr++=0;
	return;
}

if((uint8_t) *amlptr == '\\') {		/* root character */
	*bufptr++='\\';
	amlptr++;
}

if((uint8_t) *amlptr++ == 0x2e) {		/* DualNamePrefix */
	NumberOfSegments=2;
}
if((uint8_t) *amlptr == 0x2f) {		/* MultiNamePrefix */
	NumberOfSegments=(uint8_t) *++amlptr;
}
else			/* single segment */
{
	NumberOfSegments=1;
}

do {
	memcpy(bufptr,amlptr,AML_STRING_SEGMENT_SIZE);		/* copy data */

	bufptr += AML_STRING_SEGMENT_SIZE;
} while(NumberOfSegments-- > 0);

return(amlptr);
}

/*
* Add subnode to ACPI device tree
*
* In: Type	Node type
      dataptr	Pointer to data copy, NULL if none
*
*  Returns: NULL on error, pointer to next AML term on success
*
*/

size_t ACPIAddSubnode(size_t type,char *name,AMLTREEVALUE *NodeData) {
if(ACPICurrentScope == NULL) {		/* first node in list */

	/* create root node */
	ACPICurrentScope=kernelalloc(sizeof(ACPITreeNode));
	if(ACPICurrentScope == NULL) return(-1);

	ACPIRoot=ACPICurrentScope;
	ACPIParentScope=ACPICurrentScope;
}
else
{
	ACPICurrentScope->subnodes[ACPICurrentScope->NumberOfSubNodes]=kernelalloc(sizeof(ACPITreeNode));
	if(ACPICurrentScope->subnodes[ACPICurrentScope->NumberOfSubNodes++] == NULL) return(-1);

	ACPICurrentScope=ACPICurrentScope->subnodes[ACPICurrentScope->NumberOfSubNodes++];	 /* set current scope */
}

ACPICurrentScope->type=type;		/* set node type */

memcpy(ACPICurrentScope->data,NodeData,sizeof(AMLTREEVALUE));	/* copy node data */

if(ACPICurrentScope->data != NULL) memcpy(ACPICurrentScope->data,dataptr,datalen);	/* copy node data */

return(0);
}

/*
* Parse AML to ACPI tree
*
* In: aml_data	Pointer to AML data
*
*  Returns: NULL on error, pointer to next AML term on success
*
*/
uint8_t *ParseAML(uint8_t *aml_data) {
size_t BlockSize=0;
uint8_t *amlptr=aml_data;
uint8_t *endamlptr;
char *AMLName[MAX_SIZE];
uint8_t AMLZeroOp=0;
uint8_t AMLOneOp=1;
AMLTREEVALUE NodeData;

/* check if term allowed in this block */

if(((uint8_t) *amlptr == ACPI_DEVICE_BLOCK) ||
   ((uint8_t) *amlptr == ACPI_SCOPE_BLOCK) ||
   ((uint8_t) *amlptr == ACPI_THERMALZONE_BLOCK) ||
   ((uint8_t) *amlptr == ACPI_POWERRESOURCE_BLOCK)) {

	if((uint8_t) *amlptr == ACPI_THERMAL_ZONE) {
		if((ACPIParentScope->type != ACPI_METHOD_BLOCK) && 
		   (ACPIParentScope->type != ACPI_NAME) &&
		   (ACPIParentScope->type != ACPI_SCOPE_BLOCK) &&
        	   (ACPIParentScope->type != ACPI_POWERRESOURCE_BLOCK)) {
			kprintf_direct(InvalidNestedTerm,ACPIBlockTerms[(size_t) *amlptr],ACPIBlockTerms[ACPIParentScope->type]);
			return(NULL);
		}
	else if((uint8_t) *amlptr == ACPI_POWERRESOURCE_ZONE) {
		if((ACPIParentScope->type != ACPI_METHOD_BLOCK) && 
		   (ACPIParentScope->type != ACPI_NAME) &&
		   (ACPIParentScope->type != ACPI_SCOPE_BLOCK) &&
	           (ACPIParentScope->type != ACPI_DEVICE_BLOCK) &&
		   (ACPIParentScope->type != ACPI_PROCESSOR_BLOCK)) {
			kprintf_direct(InvalidNestedTerm,ACPIBlockTerms[(size_t) *amlptr],ACPIBlockTerms[ACPIParentScope->type]);
			return(NULL);
		}
	}

	if(((uint8_t) *amlptr == ACPI_DEVICE_BLOCK) || ((uint8_t) *amlptr == ACPI_SCOPE_BLOCK)) {
		ACPIParentScope=&ACPICurrentScope->subnodes[ACPICurrentScope->NumberOfSubNodes];	/* save parent scope */
	}

	if(((uint8_t) *amlptr == ACPI_DEVICE_BLOCK) || ((uint8_t) *amlptr == ACPI_SCOPE_BLOCK)) ACPICurrentScope=NewNode;

	/* read node length and name */

	FieldLength=AMLGetFieldLength(amlptr);	/* get node length */
	amlend=(amlptr+FieldLength);		/* get end of node */
	amlptr++;

	amlptr=GetAMLString(amlptr,AMLName);		/* get node name */

	/* Add new node for scope or device block */

	if((uint8_t) *amlptr == ACPI_DEVICE_BLOCK) {
		ACPIAddSubnode(ACPI_DEVICE_BLOCK,AMLName,NULL);
	}
	else if((uint8_t) *amlptr == ACPI_SCOPE_BLOCK) {
		ACPIAddSubnode(ACPI_SCOPE_BLOCK,AMLName,NULL);
	}

	/* add tree nodes underneath device or scope node */
 
	while(amlptr != amlend) {
		amlptr=ParseAML(amlptr);		/* parse fields within device or scope block */
		if(amlptr == NULL) return(NULL);
	}
}
else if(uint8_t) *amlptr == ACPI_METHOD_BLOCK) {
	if(ACPICurrentScope == NULL) {		/* not in scope */
		kprintf_direct(ACPITermOutsideBlock,ACPIBlockTerms[ACPI_METHOD_BLOCK]);
		return(NULL);
	}
	
	ACPIParentScope->subnodes[ACPIParentScope->NumberOfSubNodes]->type=ACPI_METHOD_BLOCK;		/* set node type */
	amlptr++;

	/* Method blocks are processed as a single block, rather than as a series of nested tree nodes */

	ACPIParentScope->subnodes[ACPIParentScope->NumberOfSubNodes]->MethodPtr=kernelalloc(FieldLength);	/* allocate method buffer */
	if(ACPIParentScope->subnodes[ACPIParentScope->NumberOfSubNodes]->MethodPtr == NULL) return(NULL);

	memcpy(ACPIParentScope->subnodes[ACPIParentScope->NumberOfSubNodes]->data,amlptr,FieldLength-1);	/* copy method */

	ACPIParentScope->subnodes[ACPIParentScope->NumberOfSubNodes]->MethodSize=FieldLength-1;	/* set method buffer size */

	FieldLength--;
	amlptr += FieldLength;		/* skip over field */
}
else if((uint8_t) *amlptr == ACPI_EXTENDED_OP) {
	amlptr++;
	FieldLength--;
}	
else if((uint8_t) *++amlptr == ACPI_NAME_OP) {	/* name */
	if(ACPICurrentScope == NULL) {		/* not in scope */
		kprintf_direct(ACPITermOutsideBlock,ACPIBlockTerms[ACPI_NAME]);
		return(NULL);
	}

	if((uint8_t) *++amlptr == AML_NAME_SEG) {
		oldamlptr=amlptr;
		amlptr=GetAMLString(amlptr,AMLName);

		strncpy(NodeData.string,AMLName,MAX_SIZE);	/* copy name */

		NodeData.type=AML_WORD;
		NodeData.WordData=(uint16_t) *amlptr++;

		if((uint8_t) *++amlptr == AML_ZERO_OP) {		/* byte 0 constant */
			NodeData.type=AML_BYTE;
			NodeData.ByteData=0;

			FieldLength--;
		}
		else if((uint8_t) *++amlptr == AML_ONE_OP) {		/* byte 1 constant */
			NodeData.type=AML_BYTE;
			NodeData.ByteData=0;

			FieldLength--;
		}
		else if((uint8_t) *++amlptr == AML_BYTE_PREFIX) {	/* byte */
			NodeData.type=AML_BYTE;
			NodeData.ByteData=*amlptr++;

			FieldLength--;
		}
		else if((uint8_t) *++amlptr == AML_WORD_PREFIX) {	/* word */
			NodeData.type=AML_WORD;
			NodeData.WordData=(uint16_t) *amlptr;

			amlptr += 2;
			FieldLength -= 2;
		}
		else if((uint8_t) *++amlptr == AML_DWORD_PREFIX) {	/* dword */
			NodeData.type=AML_DWORD;
			NodeData.WordData=(uint32_t) *amlptr;

			amlptr += 4;
			FieldLength -= 4;
		}
		else if((uint8_t) *++amlptr == AML_QWORD_PREFIX) {	/* qword */
			NodeData.type=AML_QWORD;
			NodeData.QwordData=(uint64_t) *amlptr;

			amlptr += 8;
			FieldLength -= 8;
		}

		ACPIAddSubnode(NodeData.type,AMLName,&NodeData);	
	}
}

return(amlptr);
}

/*
* Execute AML Method
*
* In: MethodName		Method name
*	args			Arguments

* Returns: Nothing
*
*/
size_t ExecuteAMLMethod(char *MethodName,AMLVALUE *args[6]) {
ACPITreeNode *MethodNode;
AMLVALUE local[7];

MethodNode=FindACPINode(MethodNode,GetACPIRootNode());	/* Find method node */
if(MethodNode == NULL) {		/* not found */
	SetLastError(INVALID_VALUE);
	return(-1);
}

/* push method call information onto call stack */

if(AMLMethodCallStack == NULL) {
	AMLMethodCallStack=kernelalloc(sizeof(AMLMETHODCALLSTACK));
	if(AMLMethodCallStack == NULL) {
		kprintf_direct(AMLMethodExecError,kstrerr(getlasterror()),MethodName);
		return(-1);
	}

	AMLMethodCallStack_end->prev=NULL;
}
else
{
	AMLMethodCallStack_end->next=kernelalloc(sizeof(AMLMETHODCALLSTACK));
	if(AMLMethodCallStack_end->next == NULL) {
		kprintf_direct(AMLMethodExecError,kstrerr(getlasterror()),MethodName);
		return(-1);
	}

	AMLMethodCallStack_end->next->prev=AMLMethodCallStack_end;
	AMLMethodCallStack_end=AMLMethodCallStack_end->next;
}

strcpy(AMLMethodCallStack_end->name,MethodName);
AMLMethodCallStack_end->methodptr=MethodNode->MethodPtr;		/* point to method data */

memcpy(AMLMethodCallStack_end->args,args,sizeof(AMLVALUE) * 7);		/* copy args */

ACPIParentScope=ACPICurrentScope;
ACPICurrentScope=MethodNode;


do {
	ExecuteAMLInstruction(methodptr);	/* execute AML instruction */
} while(methodptr != (methodptr + MethodNode->MethodSize));

AMLMethodCallStack_end->prev->next=AMLMethodCallStack_end->next;	/* remove from call stack */

kernelfree(AMLMethodCallStack_end);

ACPICurrentScope=ACPIParentScope;
return(0);
}

size_t ExecuteAMLInstruction(uint8_t *method) {
size_t returnvalue;
uint8_t AMLOp;
uint8_t ExtendedAMLOp=0;
uint8_t *methodptr;

methodptr=method;

if(IsTermArg((uint8_t) *methodptr)) {		/* is a TermArg */
	WriteTarget(ParseTarget(methodptr++),ParseTermArg(methodptr++));
}
else if((uint8_t) *methodptr) == 0xA0) {
	IfStatement(methodptr);
}
else if((uint8_t) *methodptr) == 0xA1) {	/* else */
	ElseStatement(methodptr);
}
else if((uint8_t) *methodptr) == 0xA2) {	/* while */
	WhileOp(methodptr);
}
else if((uint8_t) *methodptr) == 0xA3) {	/* no-op */
	break;
}

return;
}

size_t IsTermArg(uint8_t *method) {
uint8_t *methodptr;
uint8_t AMLOp;
uint8_t AMLExtendedOp;

methodptr=method;

AMLOp=(uint8_t) *methodptr++);			/* get AML operation */
if(AMLOp == 0x5B) {		/* extended AML operation */
	ExtendedAMLOp=AMLOp;																							
	AMLOp=(uint8_t) *methodptr++);			/* get AML operation */
}

if(((AMLOp >= 0x72) && (AMLOp <= 0x80)) return(TRUE);  	/* and, nand, or, nor, exclusive or, bitwise not */
if(AMLOp == 0x86) return(TRUE); 	/* modulus */
if((AMLOp >= 0x92) && (AMLOp <= 0x95)) return(TRUE); /* logical not, equals, greater than, less than */
if((ExtendedAMLOp == 0x53) && (AMLOp == 0x29)) return(TRUE);	/* ToBCDOp */
if((ExtendedAMLOp == 0x53) && (AMLOp == 0x28)) return(TRUE);	/* FromBCDOp */
if((AMLOp >= 0x96) && (AMLOp <= 0x9C)) return(TRUE);	/* ToBuffer, ToDecimalString, ToHexString, ToInteger, ToString */
if(AMLOp == 0x73) return(TRUE); 	/* concat */
if(AMLOp == 0x84) return(TRUE); 	/* concatres */
if(AMLOp == 0x87) return(TRUE); 	/* sizeof */
if(AMLOp == 0x8e) return(TRUE); 	/* ObjectType */
if(AMLOp == 0x89) return(TRUE); 	/* match */
if(AMLOp == 0x9e) return(TRUE); 	/* mid */						
if((ExtendedAMLOp == 0x53) && (AMLOp == 0x12)) return(TRUE);	/* condref */
if(AMLOp == 0x71) return(TRUE); 	/* refof */
if(AMLOp == 0x83) return(TRUE); 	/* derefof */
if(AMLOp == 0x88) return(TRUE); 	/* index */
if(AMLOp == 0) return(TRUE); 	/* zero */
if(AMLOp == 1) return(TRUE); 	/* one */
if(AMLOp == 0xFF) return(TRUE); 	/* ones */
if((ExtendedAMLOp == 0x53) && (AMLOp == 0x30)) return(TRUE);	/* revision */
if(AMLOp == AML_BYTE_PREFIX) return(TRUE);
if(AMLOp == AML_WORD_PREFIX) return(TRUE);
if(AMLOp == AML_DWORD_PREFIX) return(TRUE);
if(AMLOp == AML_QWORD_PREFIX) return(TRUE);
if((ExtendedAMLOp == 0x53) && (AMLOp == 0x23)) return(TRUE);	/* acquire */
if((ExtendedAMLOp == 0x53) && (AMLOp == 0x25)) return(TRUE);	/* wait */
if(AMLOp == 0x70) return(TRUE);		/* store */
if(AMLOp == 0x14) return(TRUE);		/* method */

return(FALSE);
}

/*
* AML if operation
*
* In: methodptr		Pointer to current AML address
*
* Returns: Nothing
*
*/
void IfStatement(uint8_t *methodptr) {
size_t pkglength;
size_t FoundOp=FALSE;

pkglength=(size_t) *methodptr++;			/* get package length */

if(EvaluatePredicate(methodptr++) == TRUE) {	/* if predicate is true */
	do {
		ExecuteAMLInstruction(methodptr);
	} while(methodptr != (methodptr + pkglengfth));
}
else						/* predicate is false */
{
	do {
		if(((uint8_t) *methodptr == AML_ELSE_OP) && (FoundElse == FALSE)) FoundElse=TRUE;	/* found ElseOp */

		if(FoundElse == TRUE) ExecuteAMLInstruction(methodptr);

	} while(methodptr != (methodptr + pkglengfth));

return;
}

/*
* AML while operation
*
* In: methodptr		Pointer to current AML address
*
* Returns: Nothing
*
*/
void WhileOp(uint8_t *methodptr) {
size_t pkglength;
uint8_t predicate;

pkglength=(size_t) *methodptr++;			/* get package length */
predicate=*methodptr++;

while(EvaluatePredicate(predicate) == TRUE) {	/* if predicate is true */
	do {
		if((uint8_t) *methodptr == AML_BREAK_OP) {	/* BreakOp */
			return;
		}
		else if((uint8_t) *methodptr == AML_CONTINUE_OP) {	/* ContinueOp */
			methodptr++;
			continue;
		}
		else
		{
			ExecuteAMLInstruction(methodptr);
		}
	} while(methodptr != (methodptr + pkglengfth));
}

return;
}

/*
* Update target with value
*
* In: amlptr		Pointer to current AML address
	value		Value
*
* Returns: Nothing
*
*/

void WriteTarget(uint8_t *amlptr,size_t value) {
ACPITreeNode *node;

if(((uint8_t) *amlptr >= 0x60) && ((uint8_t) *amlptr <= 0x67)	{ /* local0 ... local7 */
	AMLMethodCallStack_end->local[target]=value;
	
	return;
}
else if(((uint8_t) *amlptr >= 0x68) && ((uint8_t) *amlptr <= 0x6E)	{ /* arg0 ... arg7 */
	AMLMethodCallStack_end->arg[target]=value;
	
	return;
}
else if((uint8_t) *amlptr == 0x8) {		/* name */
	amlptr++;

	amlptr=GetAMLString(amlptr,AMLName);	/* get target name */
	node=FindACPINode(AMLNAME);		/* find ACPI node */

	if((uint8_t) *++amlptr == AML_ZERO_OP) {		/* byte 0 constant */
		node->ByteData=0;
	}
	else if((uint8_t) *++amlptr == AML_ONE_OP) {		/* byte 1 constant */
		node->ByteData=1;
	}
	else if((uint8_t) *++amlptr == AML_BYTE_PREFIX) {	/* byte */
		node->ByteData=*amlptr++;
	}
	else if((uint8_t) *++amlptr == AML_WORD_PREFIX) {	/* word */
		node->WordData=(uint16_t) *amlptr;

		amlptr += 2;
	}
	else if((uint8_t) *++amlptr == AML_DWORD_PREFIX) {	/* dword */
		node->WordData=(uint32_t) *amlptr;

		amlptr += 4;
	}
	else if((uint8_t) *++amlptr == AML_QWORD_PREFIX) {	/* qword */
		node->QwordData=(uint64_t) *amlptr++;

		amlptr += 8;
	}
}

return;
}

size_t ParseTermArg(uint8_t *amlptr) {
ACPITreeNode *node;

if(((uint8_t) *amlptr >= 0x60) && ((uint8_t) *amlptr <= 0x67)) {		/* local0 - local7 */
	return(AMLMethodCallStack_end->local[(size_t) termbyte]);
}
else if((uint8_t) *amlptr == 0x8) {		/* name */
	amlptr=GetAMLString(amlptr,AMLName);	/* get target name */
	node=FindACPINode(AMLNAME);		/* find ACPI node */
}
else if((uint8_t) *amlptr == 0x70) {	/* store */
	ParseTermArg(++amlptr);
}
else if((uint8_t) *amlptr == 0x72) {	/* add */
	return(ParseTarget(amlptr++),ParseTermArg(++amlptr) + ParseTermArg(++amlptr));
}
else if((uint8_t) *amlptr == 0x74) {		/* subtract */
	return(ParseTermArg(++amlptr));
}
else if((uint8_t) *amlptr == 0x75) {		/* increment */
	returnvalue=ParseTermArg(++amlptr);

	return(++returnvalue);
}
else if((uint8_t) *amlptr == 0x76) {		/* decrement */
	returnvalue=ParseTermArg(++amlptr);

	return(--returnvalue);
}
else if((uint8_t) *amlptr == 0x77) {		/* multiply */
	return(ParseTermArg(++amlptr) * ParseTermArg(++amlptr));
}
else if((uint8_t) *amlptr == 0x78) {		/* divide */
	return(ParseTermArg(++amlptr) / ParseTermArg(++amlptr));
}
else if((uint8_t) *amlptr == 0x79) {		/* shift left */
	return(ParseTermArg(++amlptr) << ParseTermArg(++amlptr));
}
else if((uint8_t) *amlptr == 0x7A) {		/* shift right */
	return(ParseTermArg(++amlptr) >> ParseTermArg(++amlptr));
}
else if((uint8_t) *amlptr == 0x7B) {		/* logical and */
	return(ParseTermArg(++amlptr) & ParseTermArg(++amlptr));
}
else if((uint8_t) *amlptr == 0x7C) {		/* logical nand */
	return(~(ParseTermArg(++amlptr) & ParseTermArg(++amlptr)));
}
else if((uint8_t) *amlptr == 0x7D) {		/* logical or */
	return(ParseTermArg(++amlptr) | ParseTermArg(++amlptr));
}
else if((uint8_t) *amlptr == 0x7E) {		/* logical nor */
	ParseTermArg(++amlptr),~(ParseTermArg(++amlptr) | ParseTermArg(++amlptr)));
}
else if((uint8_t) *amlptr == 0x7F) {		/* logical exclusive or */
	return(ParseTermArg(++amlptr) ^ ParseTermArg(++amlptr));
}
else if((uint8_t) *amlptr == 0x80) {		/* bitwise not */
	ParseTermArg(++amlptr),~ParseTermArg(++amlptr));
}
else if((uint8_t) *amlptr == 0x86) {		/* modulus */
	return(ParseTermArg(++amlptr) % ParseTermArg(++amlptr));
}
else if((uint8_t) *amlptr == 0x92) {		/* logical not */
	return(!ParseTermArg(++amlptr));
}
else if((uint8_t) *amlptr == 0x93) {		/* logical equals */
	return(ParseTermArg(++amlptr) == ParseTermArg(++amlptr));
}
else if((uint8_t) *amlptr == 0x94) {		/* logical greater than */
	return(ParseTermArg(++amlptr) > ParseTermArg(++amlptr));
}
else if((uint8_t) *amlptr == 0x95) {		/* logical less than */
	return(ParseTermArg(++amlptr) < ParseTermArg(++amlptr));
}
else if((uint8_t) *amlptr == 0) {		/* zero */
	return(0);
}
else if((uint8_t) *amlptr == 1) {		/* one */
	return(1);
}
else if((uint8_t) *amlptr == 0xFF) {		/* ones */
	return(0xFF);
}
else if((uint8_t) *amlptr == AML_BYTE_PREFIX) {	/* byte */
	return((uint8_t) *++amlptr);
}


