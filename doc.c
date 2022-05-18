#include "doc.h"

/*	1  byte - data field type (0x0c for str and 0x0d for num),
 *	1 byte - data field size in bytes, 128 bytes - data field
 */
#define	TLV_FRAME_MAXLEN	130

Doc *new_doc(FILE *docf) {
	if (docf == NULL) {
		fprintf(stderr, "docf == NULL\n");
		return NULL;
	}

	Doc *ret = (Doc*)calloc(1, sizeof(Doc));
	if (!ret) {
		perror("malloc");
		return NULL;
	}
	ret->file = docf;
	if (doc_parse(ret)) {
		perror("doc_parse");
	}
	return ret;
}

Node *doc_new_root(Doc *doc, Node *tmp) {
	doc->roots[doc->numroots] = tmp;
	doc->numroots++;
	doc->roots = (Node**)realloc(doc->roots,
			sizeof(Node*) * (doc->numroots + 1));
	doc->roots[doc->numroots] = NULL;
	return doc->roots[0];
}

Node *new_node(char *n) {
	Node *ret = (Node*)calloc(1, sizeof(Node));
	ret->name = strdup(n);
	ret->numchildren = 0;
	ret->children = (Child**)calloc(1, sizeof(Child*));
	ret->numattribs = 0;
	ret->attribs = (Attrib**)calloc(1, sizeof(Attrib*));
	return ret;
}

Node *node_add_child(Node *parent, Node *child, char single) {
	if (!parent || !child)
		return NULL;

	child->single = single;

	parent->children[parent->numchildren] = (Child*)
						calloc(1, sizeof(Child));
	parent->children[parent->numchildren]->u.n = child;
	parent->children[parent->numchildren]->type = NODE;
	parent->numchildren++;
	parent->children = realloc(parent->children,
				(parent->numchildren + 1) * sizeof(Child*));
	parent->children[parent->numchildren] = NULL;

	child->parent = parent;
	return child;
}

Node *node_add_attrib(Node *n, char *name, char *value) {
	if (!n || !name || !value)
		return n;

	n->attribs[n->numattribs] = (Attrib*)calloc(1, sizeof(Attrib*));
	n->attribs[n->numattribs]->name = name;
	n->attribs[n->numattribs]->value = value;
	n->numattribs++;
	n->attribs = realloc(n->attribs, (n->numattribs + 1) * sizeof(Attrib*));
	n->attribs[n->numattribs] = NULL;

	return n;
}

Node *node_add_text(Node *parent, char *txt) {
	if (!parent || !txt || !strlen(txt))
		return parent;

	parent->children[parent->numchildren] = (Child*)
						calloc(1, sizeof(Child));
	parent->children[parent->numchildren]->u.c = strdup(txt);
	parent->children[parent->numchildren]->type = CSTR;
	parent->numchildren++;
	parent->children = realloc(parent->children,
				(parent->numchildren + 1) * sizeof(Child*));
	parent->children[parent->numchildren] = NULL;

	return parent;
}

FILE * output_file;

void parsenode(Node *n) {
	int i;

	char strbuf[TLV_FRAME_MAXLEN];

	for (i = 0; n->children[i]; i++) {
		if(!strcmp(n->name,(const char* )"numeric")) {
			int paresed_num = atoi(n->children[i]->u.c);

			strbuf[0] = 0x0d;
			strbuf[1] = sizeof(paresed_num);
			strbuf[2] = (paresed_num >> 24) &0xff;
			strbuf[3] = (paresed_num >> 16) &0xff;
			strbuf[4] = (paresed_num >> 8) &0xff;
			strbuf[5] = paresed_num &0xff;

			fwrite(strbuf, sizeof(int) + 2, 1, output_file);

		}
		else if(!strcmp(n->name,(const char* )"text")) { 					
			strbuf[0] = 0x0c;
			strbuf[1] = strlen(n->children[i]->u.c);
			memcpy((void*) &strbuf[2], (void*) n->children[i]->u.c, strlen(n->children[i]->u.c));

			fwrite(strbuf, strlen(n->children[i]->u.c) + 2, 1, output_file);
		}		
		
		if (n->children[i]->type == NODE)
			parsenode(n->children[i]->u.n);
	}		
}

void parsexml(Doc *doc) {
	int j;
	if (!doc)
		return;
	
	output_file = fopen("tlv", "w");
	
	for (j = 0; doc->roots[j]; j++) {
		parsenode(doc->roots[j]);
	}
	
	fclose(output_file);	
}
