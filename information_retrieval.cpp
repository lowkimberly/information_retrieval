#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <string>
#include <ctype.h>
#include <vector>
#include <algorithm>

using namespace std;

struct posting_triple {
	char term[256];
	int doc_id;
	int doc_pos;
};


//sort by term, doc id, finally pos
bool triple_comp (struct posting_triple a, struct posting_triple b) {
	if (strcmp(a.term, b.term) < 0 ){
		return true;
	}
	else if (strcmp(a.term, b.term) > 0 ) {
		return false;
	}
	else if (a.doc_id < b.doc_id) {
		return true;
	}
	else if (a.doc_id> b.doc_id ) {
		return false;
	}
	else return a.doc_pos < b.doc_pos;
}

int main () {
	FILE * p_data;
	FILE * p_dict;
	FILE * p_post;

	vector<string> dict_vector;
	vector<posting_triple> post_vector;
	
	p_data = fopen ("dataP1.txt","r"); //this file is reading. MUST BE IN SAME DIRECTORY.
  
	if ( p_data == NULL)  {
		printf("ERROR OPENING DATA FILE\n");
		return 1;
	}
	
	//*TEXT 021 01/04/63 PAGE 022
	char line_buf[256];
	int doc_id = 0; //this will be the CURRENT docid. it looks like it starts with a 0 in the text, which won't appear when I convert to int
	int doc_pos = 0; //position in current document

	while (feof(p_data) == 0) {
		fgets(line_buf,256,p_data); //every new text is on a line by itself

		if (strncmp(line_buf,"*TEXT ", 6) == 0) {
			doc_id = atoi(string(line_buf).substr(6,3).c_str()); //id starts at position 6, and is 3 digits
			doc_pos = 0; //reset from last turn.
		}
		
		else {
			//working with an actual line
			char * tok;
			tok = strtok (line_buf," ,\"\n()!&:;?-/"); 
			while (tok != NULL) {
				//if has a length, and not a period by itself
				if (strlen(tok) > 0 && strcmp(tok,".") != 0) {
					//has a period, not by itself. Copy all the stuff after it (.INVESTIGATING).
					if (strncmp(tok,".", 1) == 0) {
						strcpy( tok, string(tok).substr(1,strlen(tok)).c_str() );	
					}
					
					dict_vector.push_back(tok); //push back ALL the tokens in the dictionary, I'll deal with duplicates later

					//add to postings triple term, docid, position in document
					posting_triple pt;
					strcpy(pt.term,tok);
					pt.doc_id = doc_id;
					pt.doc_pos = doc_pos;
					post_vector.push_back(pt);

					doc_pos++;
				}
				tok = strtok (NULL, " ,\"\n()!&:;?-/");
			}
		}		
	}

	fclose(p_data);
	
	//read and write dictionary and postings
	p_dict = fopen ("dict.txt","w");
	p_post = fopen ("post.txt","w");
	if ( p_dict == NULL || p_data==NULL)  {
		printf("ERROR OPENING OUTPUT FILES\n");
		return 1;
	}

	sort(dict_vector.begin(), dict_vector.end());
	for (int d = 0; d < dict_vector.size() ; d++) {
		//if first item or the current item is different from the previous one (skip duplicates)
		if (d == 0 || (d > 0 && strcmp(dict_vector[d].c_str(), dict_vector[d-1].c_str()) != 0 ) ) {
			fprintf(p_dict,"%s\n",dict_vector[d].c_str());
		}
	}

	sort(post_vector.begin(), post_vector.end(),triple_comp);	
	for (int p = 0; p < post_vector.size() ; p++) fprintf(p_post,"%s %d %d\n",post_vector[p].term,post_vector[p].doc_id, post_vector[p].doc_pos);
	
	
	fclose(p_dict);
	fclose(p_post);	
	return 0;
}
