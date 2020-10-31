
#include <stdio.h>
#include <iostream>
#include <vector>
#include <stdlib.h>
#include <string>
#include <string.h>

using namespace std;

struct dict_triple { //everything we need for the new dictionary
	char term[256];
	int doc_freq;
	int post_pos;
};

int main () {
	FILE * p_dict;
	FILE * p_post;
	vector<dict_triple> new_dict;
	
	p_dict = fopen ("dict.txt","r"); //open previously generated dictionary for reading
	if ( p_dict == NULL)  {
		printf("ERROR OPENING DICTIONARY FILE\n");
		return 1;
	}

	char line_buf[256];

	while (feof(p_dict) == 0) {
		fgets(line_buf,256,p_dict);
		if (feof(p_dict) != 0) break; //eof after retreiving? Since there's a newline at the end, break here.
		dict_triple dt;
		string str_line_buf(line_buf);
		strcpy(dt.term, 	str_line_buf.substr(0, str_line_buf.length() - 1).c_str()  ); //erase newline at end
		dt.doc_freq = 0;
		dt.post_pos = 0; //first location in postings
		new_dict.push_back(dt);
	}
	fclose(p_dict);

	int triple_num = 0; //number of the original posting
	int term_num = 0; //vector index
	
	p_post = fopen ("post.txt","r"); 
	if ( p_post == NULL)  {
		printf("ERROR OPENING POSTINGS FILE\n");
		return 1;
	}

	char newest_term[256];
	char prev_term[256];
	int prev_docid = -1;

	while (feof(p_post) == 0) {
		fgets(line_buf,256,p_post); //get next line in postings
		if (feof(p_post) != 0) break; //did not realize my previous code printed a newline at the end of the file. Should have noticed.
		//SOME_TERM_HERE 21 474
		//always have 3 values
		char * tempterm; //term
		int tempid = -1;
		int temppos = -1;

		tempterm = strtok (line_buf," "); //term
		strcpy(newest_term,tempterm);
		tempid = atoi(strtok (NULL, " ")); //doc id
		temppos = atoi(strtok (NULL, " ")); //ois
	
		if (triple_num == 0) { //very first in postings list.
			strcpy(prev_term, newest_term);
			prev_docid=-1;
		}
		
		if 	 (strcmp(newest_term,prev_term) == 0 ) { //same term as previously?
			if ( tempid != prev_docid) { //but a different document?
			//we have the index for the term in the vector, if everything is sorted. 
				new_dict[term_num].doc_freq++;
				prev_docid = tempid;
			}
		}		
		else { //new term
			term_num++;
			new_dict[term_num].post_pos = triple_num; //this is like the very first pos we find in the document.
			new_dict[term_num].doc_freq = 1;
			strcpy(prev_term,newest_term); 
			prev_docid = tempid;
		}
		
		triple_num++; //next posting
	}
	fclose(p_post);
	
	FILE * new_d;
	new_d=fopen ("new_dict.txt","w"); 
	for (int i = 0; i < new_dict.size(); i++ ){
		fprintf(new_d, "%s %d %d\n", new_dict[i].term, new_dict[i].doc_freq, new_dict[i].post_pos );
	}
	fclose(new_d);
	return 0;
}
