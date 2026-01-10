#include <stdint.h>
#include <stddef.h>
#include "acpi.h"
#include "aml.h"

ACPITreeNode *ACPIRoot=NULL;
ACPITreeNode *ACPICurrentScope=NULL;
ACPITreeNode *ACPIParentScope=NULL;
char *InvalidNestedTerm="acpi: Invalid nested AML term %s in %s block\n";
char *ACPITermOutsideBlock="acpi: AML %s term outside of DEVICE or SCOPE block\n";
char *ACPITerms[] = { "Device","Scope","Processor","ThermalZone","PowerResource","Method" };

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
ACPITreeNode *FindACPINode(char *name,ACPITreeNode *node) {
size_t NodeCount=0;
bool IsFound=FALSE;
ACPITreeNode *FoundNode;

/* search through nodes */

for(NodeCount=0;NodeCount < node->NumberOfSubNodes;NodeCount++) {
	if((node->data != NULL) && (strncmp(node->data,name) == 0) return(node);
	
	FoundNode=FindACPIDevice(name,&node->subnode[NodeCount]);	/* find sub-node */
	if(FoundNode != NULL) return(FoundNode);			/* found sub-node */
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
	FieldLength += (*amlbuf << ShiftCount);
	ShiftCount += 8;
while(NumberOfLengthBytes-- > 0);

return(FieldLength);
}

/*
* Get AML string
*
* In: amlbuf	Pointer to AML data
      buf	Buffer

*  Returns: Nothing
*
*/
void GetAMLString(uint8_t *amlbuf,char *buf) {
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

if(((uint8_t) *amlptr++ == 0x2e) || (uint8_t) *amlptr++ == 0x2f)) {		/* DualNamePrefix or MultiNamePrefix */
	if((uint8_t) *amlptr++ == 0x2e) {
		NumberOfSegments=2;
	}
	else
	{
		NumberOfSegments=*amlptr++;
	}

	
	do {
		memcpy(bufptr,amlptr,AML_STRING_SEGMENT_SIZE);		/* copy data */

		bufptr += AML_STRING_SEGMENT_SIZE;
		
	} while(NumberOfSegments-- > 0);
}
else			/* single segment */
{
	memcpy(bufptr,amlptr,AML_STRING_SEGMENT_SIZE);		/* copy data */
}

return;
}

/*
* Add subnode to ACPI device tree
*
* In: Type	Node type
      dataptr	Pointer to data copy, NULL if none
      datalen	Length of data

*  Returns: NULL on error, pointer to next AML term on success
*
*/
size_t ACPIAddSubnode(size_t type,AMLTREEVALUE NodeData) {
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

	if(*amlptr == ACPI_THERMAL_ZONE) {
		if((ACPIParentScope->type != ACPI_METHOD_BLOCK) && 
		   (ACPIParentScope->type != ACPI_NAME) &&
		   (ACPIParentScope->type != ACPI_SCOPE_BLOCK) &&
        	   (ACPIParentScope->type != ACPI_POWERRESOURCE_BLOCK)) {
			kprintf_direct(InvalidNestedTerm,ACPIBlockTerms[(size_t) *amlptr],ACPIBlockTerms[ACPIParentScope->type]);
			return(NULL);
		}
	else if(*amlptr == ACPI_POWERRESOURCE_ZONE) {
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
		ACPIAddSubnode(ACPI_DEVICE_BLOCK,AMLName,strlen(AMLName));
	}
	else if((uint8_t) *amlptr == ACPI_SCOPE_BLOCK) {
		ACPIAddSubnode(ACPI_SCOPE_BLOCK,AMLName,strlen(AMLName));
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

		ACPIAddSubnode(NodeData.type,&NodeData);
	
		FieldLength -= (amlptr-oldamlptr);		/* subtract size of name segments */
	}
	else if((uint8_t) *++amlptr == AML_ZERO_OP) {		/* byte 0 constant */
		NodeData.type=AML_BYTE;
		NodeData.ByteData=0;

		ACPIAddSubnode(NodeData.type,&NodeData);

		FieldLength--;
	}
	else if((uint8_t) *++amlptr == AML_ONE_OP) {		/* byte 1 constant */
		NodeData.type=AML_BYTE;
		NodeData.ByteData=0;

		ACPIAddSubnode(NodeData.type,&NodeData);

		FieldLength--;
	}
	else if((uint8_t) *++amlptr == AML_BYTE_PREFIX) {	/* byte */
		NodeData.type=AML_BYTE;
		NodeData.ByteData=*amlptr++;

		ACPIAddSubnode(NodeData.type,&NodeData);

		FieldLength--;
	}
	else if((uint8_t) *++amlptr == AML_WORD_PREFIX) {	/* word */
		NodeData.type=AML_WORD;
		NodeData.WordData=(uint16_t) *amlptr++;

		ACPIAddSubnode(NodeData.type,&NodeData);

		FieldLength -= 2;
	}
	else if((uint8_t) *++amlptr == AML_DWORD_PREFIX) {	/* dword */
		NodeData.type=AML_DWORD;
		NodeData.WordData=(uint32_t) *amlptr++;

		ACPIAddSubnode(NodeData.type,&NodeData);

		FieldLength -= 4;
	}
	else if((uint8_t) *++amlptr == AML_QWORD_PREFIX) {	/* qword */
		NodeData.type=AML_QWORD;
		NodeData.QwordData=(uint64_t) *amlptr++;

		ACPIAddSubnode(NodeData.type,&NodeData);

		FieldLength -= 8;
	}
}

return(amlptr);
}

size_t ExecuteAMLMethod(char *MethodName,AMLVALUE *args[6]) {
ACPITreeNode *MethodNode;
uint8_t *methodptr;
size_t count;
AMLVALUE local[7];
size_t ProgramCounter=0;

MethodNode=FindACPINode(char *MethodNode,GetACPIRootNode());	/* Find method node */
if(MethodNode == NULL) {		/* not found */
	SetLastError(INVALID_VALUE);
	return(-1);
}

methodptr=MethodNode->MethodPtr;		/* point to method data */
count=MethodNode->MethodSize;

do {
	/* execute AML method opcode */

	switch((uint8_t) *methodptr++) {
		case 0x70:		/* modulus */
			
	}

} while(count-- > 0);

return(0);
}	 	
