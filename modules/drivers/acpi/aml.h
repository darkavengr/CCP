#include <stdint.h>
#include <stddef.h>

#define ACPI_NODE_SCOPE		1
#define ACPI_NODE_DEVICE	2
#define ACPI_NODE_METHOD	3
#define ACPI_NODE_NAME		4
#define ACPI_NODE_BYTE		5
#define ACPI_NODE_WORD		6
#define ACPI_NODE_DWORD		7
#define ACPI_NODE_QWORD		8

#define MAX_ACPI_SUBNODES	255
#define ACPI_NAME_MAX		255
#define AML_STRING_SEGMENT_SIZE	4

uint8_t *ParseAML(uint8_t *aml_data);
size_t AddSubNode(size_t type,void *dataptr,size_t datalen);
void GetAMLString(uint8_t *amlbuf,char *buf);
size_t AMLGetFieldLength(uint8_t *amlbuf);
ACPITreeNode *FindACPINode(char *name,ACPITreeNode *node);
ACPITreeNode *GetACPIRootNode(void);

#ifndef AML_H
	#define AML_H
	typedef struct {
		size_t type;
		uint8_t ByteData;
		uint16_t WordData;
		uint32_t DwordData;
		uint64_t QwordData;
		char *string[ACPI_NAME_MAX];	
	} AMLTREEVALUE;

	typedef struct {
		size_t NumberOfSubnodes;
		AMLTREEVALUE data;
		size_t MethodSize;
		void *MethodPtr;
		struct ACPINode *subnodes[MAX_ACPI_SUBNODES];
	} ACPINodeTree;

	typedef struct {
		uint8_t *buffer;
		size_t BufferSize;
	} AMLBUFFER;

	typedef struct {
		size_t integer;
		AMBUFFER buffer;
		PACKAGE package;
		FIELDUNIT fieldunit;
	} AMLTYPE;
}
#endif

