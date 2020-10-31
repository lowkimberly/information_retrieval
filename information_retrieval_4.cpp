#include <stdio.h>
#include <iostream>
#include <vector>
#include <map>
#include <math.h>
#include <sstream>

using namespace std;
//from: http://www.cplusplus.com/forum/beginner/7777/
string convertInt(int number)
{
   stringstream ss;//create a stringstream
   ss << number;//add number to the stream
   return ss.str();//return a string with the contents of the stream
}
//--------
struct query_item { //one query item
	int id;
	vector<string> words;
};

struct rel_item { 
	int query_id;
	vector<int> doc_nums;
};

struct newdict_item { 
	char term[256];
	int df;
	int pos;
};

struct posting_item { 
	char term[256];
	int docid;
	int pos;
};

struct doc_score { 
	int docid;
	double score;
};

//for sorting the final results.
bool comp_doc_score(const doc_score &a, const doc_score &b) {
    return a.score > b.score;
}

int load_q(char* filename, vector<query_item> &output, vector<string> &unmodified) {
	FILE * p_q;
	
	char line_buf[256]; //for holding fgets data
	int id_begin = 0;
	bool in_phrase = false;
	bool started_phrase = false;
	
	char* valid = " ABCDEFGHIJKLMNOPQRSTUVWXYZ\"'.0123456789";


	p_q = fopen (filename,"r"); //query files
	if ( p_q == NULL)  {
		printf("Cannot open q.txt\n");
		return 1;
	}

	string stopwords("|THE|OF|AND|TO|A|IN|IS|WAS|THE|HE|"); //caps because all the words are caps

	while (feof(p_q) == 0) {
		fgets(line_buf,256,p_q); //every new text is on a line by itself
		//it's a new query. save the query id
		if (strncmp(line_buf,"*FIND", 5) == 0) {
			unmodified.push_back("");
			string newquery(line_buf); //turn into a string to search for query id
			int id_begin = newquery.find_first_not_of(" ");

			struct query_item id_only;
			id_only.id = atoi(newquery.substr(id_begin,string::npos).c_str());
			output.push_back(id_only);
			in_phrase = false;
			started_phrase = false;
		}
//------------------------------------------------------------
		//actual content
		else if (strlen(line_buf) > 1){ //skip if only a new line
			unmodified[unmodified.size()-1].append(line_buf);

			char * token;
			token = strtok (line_buf," \n");
			//every token in the line
			while (token!=NULL) {
				//strip out all the weirdo characters
				string tok_str(token);
				int t_begin = tok_str.find_first_of(valid,0);
				int t_end = tok_str.find_last_of(valid,string::npos);
				if (t_begin < t_end) 	tok_str.assign(tok_str.substr(t_begin,t_end-t_begin+1)) ; //if valid, remove the weird characters

				//at least 1 nonspace character
				if (tok_str.size() > 0	) {
					int last_added_index = output[output.size()-1].words.size() - 1; //note the minus 1

					//We have a token. First check whether it starts with a quote.
					if (strncmp(tok_str.c_str(), "\"", 1) == 0 && in_phrase == false) in_phrase = true;
				
					//still not in a phrase? just add a word
					if (in_phrase == false) {
						if (stopwords.find(tok_str.c_str(), 0, tok_str.size()) ==string::npos ) output[output.size()-1].words.push_back(tok_str);
					}

					//inside a phrase? add it to the previous item, unless it is the first.
					else if (in_phrase == true ) {
						int last_added_index = output[output.size()-1].words.size() - 1; //note the minus 1

						if (started_phrase == false)	 {
							output[output.size()-1].words.push_back(tok_str); //in phrase, but nothing stored yet. add first word.
						}
						else { //in phrase, and there's already a word in the phrase.
							output[output.size()-1].words[last_added_index].append (" " );
							output[output.size()-1].words[last_added_index].append (tok_str);
						
						}
						//definitely inside the phrase right now
						started_phrase = true;
					}
					
					//processed token, now check if we're done with the phrase or not
					//are we in the phrase? Does the token end with a quote?
					//now go back and add all those other words
					if ((tok_str[ tok_str.size()-1 ] == '"' ) && in_phrase == true) {
						in_phrase = false;
						started_phrase = false;
						//NO QUOTES HERE
						string oneword("");
						char fullphrase[512];
						strcpy (fullphrase,output[output.size()-1].words[last_added_index].c_str());
						//cannot do a nested strtok
						for (int sp = 1; sp < strlen(fullphrase); sp++) { //start at 1 to skip quote
							if (isalnum(fullphrase[sp]) !=0) {
								char c[2];
								strncpy(c,&fullphrase[sp],1);
								oneword.append(c);
							} else if (oneword.length() > 1){
								output[output.size()-1].words.push_back(oneword);
								oneword.assign("");
							}
						}	

					}
				}

			token = strtok (NULL," \n");
			}//tokenize
			if (feof(p_q) != 0) break;
		}//single line with content

	}//finished query file

}
//------------------------------------------
int load_rel(char* filename, vector<rel_item> &output) {
	FILE * p_rel;
	
	p_rel = fopen (filename,"r"); //query files
	if ( p_rel == NULL)  {
		printf("Cannot open rel.txt\n");
		return 1;
	}
	
	char line_buf[256];
	while (feof(p_rel) == 0) {
		fgets(line_buf,256,p_rel);
		if (strlen(line_buf) > 2)		{ //newline + end of string
			struct rel_item ri;
			char* token;
			token = strtok (line_buf," "); //query
			ri.query_id = atoi(token);

			token = strtok (NULL," "); //docnum

			//already something with that query? add on.
			if (output.size() > 0 && output[output.size()-1].query_id == ri.query_id) output[output.size()-1].doc_nums.push_back(atoi(token));
			else {
				ri.doc_nums.push_back(atoi(token));
				output.push_back(ri);
			}
		}
		if (feof(p_rel) != 0) break; 
	}	
}
//------------------------------------------

int load_newdict(char* filename, std::map<string,newdict_item> &output) {
//int load_newdict(char* filename, std::map<char*,newdict_item> &output) {
//word, df, pos in postings file
	FILE * p_newdict;
	
	p_newdict = fopen (filename,"r"); //query files
	if ( p_newdict == NULL)  {
		printf("Cannot open newdict.txt\n");
		return 1;
	}
	
	char line_buf[256];
	while (feof(p_newdict) == 0) {
		fgets(line_buf,256,p_newdict);
		if (feof(p_newdict) != 0) break;
		if (strlen(line_buf) > 2)		{ //newline + end of string
			struct newdict_item ndi;
			char* newterm;
			char* stringdf;
			int tempdf;
			int temppos;
			
			newterm = strtok (line_buf," "); //term
			stringdf = strtok (NULL," ");
			tempdf =  atoi(stringdf);
			temppos = atoi(strtok (NULL," "));

			strcpy(ndi.term,newterm);
			ndi.df = tempdf;
			ndi.pos = temppos;
			output[string(newterm)] = ndi;

		}
		if (feof(p_newdict) != 0) break;
	}
}

/*
dw =(how many times word appears in document)* log base 10(425/df of current word)
score for words = sum += qtf * dw
*/
double get_dw_weight(string target, int init_pos, int df, char* filename, vector <doc_score> &doc_score_list) {
//keep track of doc id. every time it changes, multiply and add
//word, docid, position
	FILE * p_posting;
	int prev_id = -1;
	double weight = 0;
	string used_ids("");
	
	p_posting = fopen (filename,"r"); //query files
	if ( p_posting == NULL)  {
		printf("Cannot open posting.txt\n");
		return 1;
	}

	char line_buf[256];
	for(int i = 0; i <= init_pos; i++) { //less or equal to, because starting with i=0
		fgets(line_buf,256,p_posting);
		if (feof(p_posting) != 0) return weight;
	}
	
	//current value in buf is what we want
	int freq = 0;
	while (feof(p_posting) == 0) {
		char* testterm;
		int docid;
		testterm = strtok (line_buf," "); //term
		docid = atoi(strtok (NULL," ")); //id

		//if word matches the word we want, but it doesn't match the old id, it's a new document.
		if (strcmp(testterm, target.c_str()) == 0 && docid != prev_id) {
			//there's a previous entry for this doc id.
			char my_id[64];
			strcpy(my_id, convertInt(prev_id).c_str());
							//find that item and add onto it
			if (prev_id > -1) {
				//every time the docid for this word changes, add the score
				struct doc_score ds;
				ds.docid = prev_id;
				ds.score = (double)freq*log10(425.0/(double)df);//currently just a weight

				int success=0;
				for(int older = 0; older <doc_score_list.size(); older++) { //check every existing docid
					if (doc_score_list[older].docid== prev_id) {
						doc_score_list[older].score+= ds.score;
						success = 1;
						break;
					}
				}
				if (success==0) doc_score_list.push_back(ds);
			}
			freq =1;
			prev_id=docid;
		}
		//if word matches the word and old id, we can add one to current freq
		else if (strcmp(testterm, target.c_str()) == 0 && docid != prev_id) {
			freq++;
		}
		else { //the terms don't match at all
			freq=0;
			prev_id=-1;
			//could be because of messed up file
			if (strcmp(testterm, target.c_str()) <0) break;
		}
		
		fgets(line_buf,256,p_posting);
		if (feof(p_posting) != 0) return weight;
	}

	fclose(p_posting);
	return weight;
}
//--------------------
double get_dpw_weight(vector<string> phrase_words, char* filename, std::map<string,newdict_item> newdict_list, vector <doc_score> &doc_score_phrases) { //phrase
//Weight for a phrase is: Number of times it appears in the document * size of the phrase
	FILE * p_posting;
	int prev_id = -1;
	int phrase_word_index = 1;//start with second word
	int occurrence = 0;
	
	p_posting = fopen (filename,"r"); //query files
	if ( p_posting == NULL)  {
		printf("Cannot open posting.txt\n");
		return 1;
	}

	//beginning word in phrase.
	string first(phrase_words[0]);
	//all docids + positions 

	char skipper[256]; //hold current data while we jump around.
	vector<int> first_id;
	vector<int> first_pos;
	for(int j = 0; j <= newdict_list[first].pos; j++) { //skip to point where the entries for the ith word are
		fgets(skipper,256,p_posting);
		if (feof(p_posting) != 0) return 0; //all the way at the end? The first word, and thus the phrase does not appear in ANY document.
	}

	
	while (feof(p_posting) == 0) {
		char* testterm;
		int docid;
		int pos;
		testterm = strtok (skipper," "); //term
		docid = atoi(strtok (NULL," ")); //id
		pos = atoi(strtok (NULL," ")); //id

		if (strcmp(testterm, first.c_str())) {
			first_id.push_back(docid);
			first_pos.push_back(pos);
		} else if (strcmp(testterm, first.c_str()) <0)  { //adjust for wrong postings
			break; //no longer matching = no more of the first word
		}
		fgets(skipper,256,p_posting);
		if (feof(p_posting) != 0) break;
	}
		
	//take each time first word occurs
	for(int i = 0; i < first_pos.size(); i++) {
		if (prev_id != first_id[i]) {
			//different id! add to doc_scores

			if (prev_id > -1) {
				struct doc_score ds;
				ds.docid = prev_id;
				ds.score = phrase_words.size() * occurrence;//currently just a weight
				doc_score_phrases.push_back(ds);
			}
				
			prev_id = first_id[i];
			occurrence=0;
		}
		//postings still exists, so let's reuse that
		rewind(p_posting);
		int offset = 0;
		int found = 0;
		int j;//outside loop
		for (j = 1; j <= phrase_words.size(); j++) { //at least 2 words in a phrase. Start with second.
			found = 0;
			offset++;
			int wanted_id = first_id[i];
			int wanted_pos = first_pos[i] + offset;
			string searching_for(phrase_words[j]);
			searching_for.append(" " + convertInt(wanted_id) + " " + convertInt(wanted_pos));

			//fgets until I find it. If I reach the end, not in this document in this position. 
			char subsequent[256];
			for(int k = 0; k <= newdict_list[first].pos; j++) { //skip to point where the entries are
				fgets(subsequent,256,p_posting);
				if (feof(p_posting) != 0) return 0; //second word is nowhere. phrase not in document
			}
			//look for exact match for skipping_for. If no match, move i on
			//if there is a match, check next j
			while (feof(p_posting) == 0) {
				if (searching_for.compare(subsequent) == 0) {
					found = 1;
					break;
				}
				fgets(subsequent,256,p_posting);
				if (feof(p_posting) != 0) break;
			}
			if (found == 0) break; //didn't find next word matching? no point in checking for the rest.
		}
		//found last word, and got all the way through j? We have one occurence in the DOCUMENT.
		if (found==1 && j > phrase_words.size()) occurrence++;
	}
	return 0;
}
//-------------------------
/*
int load_posting(char* filename, vector<posting_item> &output) {
//word, docid, position
	FILE * p_posting;
	
	p_posting = fopen ("posting.txt","r"); //query files
	if ( p_posting == NULL)  {
		printf("Cannot open posting.txt\n");
		return 1;
	}
	
	char line_buf[256];
	while (feof(p_posting) == 0) {
		fgets(line_buf,256,p_posting);
		if (feof(p_posting) != 0) break;
		if (strlen(line_buf) > 2)		{ //newline + end of string
			struct posting_item pi;
			char* newterm;
			int tempid;
			int temppos;
			
			newterm = strtok (line_buf," "); //term
			tempid =  atoi(strtok (NULL," "));
			temppos = atoi(strtok (NULL," "));

			strcpy(pi.term,newterm);
			pi.docid = tempid;
			pi.pos = temppos;
			output.push_back(pi);
		}
		if (feof(p_posting) != 0) break;
	}
}*/
//----------------------------------------------------------- main function
int main (int argc, char* argv[]) {
	//pointers
	vector<rel_item> rel_list;
	load_rel("rel.txt",rel_list);

	vector<query_item> query_list;
	vector<string> query_unmod;
	load_q("q.txt",query_list, query_unmod);

	std::map<string, newdict_item> newdict_list;
	load_newdict("newdict.txt",newdict_list);
	map<string, newdict_item>::iterator it;
	//note the positions in the postings are actually slightly off so we need to compensate.

	vector<doc_score> doc_score_list;
	vector<doc_score> doc_score_phrases;

//	vector<posting_item> posting_list;
//	load_posting("posting.txt",posting_list);

	double final_recall = 0;
	double final_precision = 0;
	double final_mean = 0;
	
	FILE * res;
	res = fopen("result.txt","w");

	for (int q = 0; q < query_list.size(); q++) { //either it's a word or phrase.
	
		for (int i = 0; i < query_list[q].words.size(); i++) {
			string this_word(query_list[q].words[i]); //individual word in query
			it = newdict_list.find(this_word);

			//if 'word' has spaces, it's a phrase. Also word must be in map.
			//single word
			if(this_word.find(" ",0) ==string::npos && it!= newdict_list.end()){
				struct newdict_item ndi = newdict_list[this_word]; //term, df, pos

				//The weight computed for each word is dw =(how many times word appears in SINGLE DOCUMENT *log base 10(425/df of the word).
				//so for every document id, do a get_dtf
				//doc ids are second value in postings

				get_dw_weight(this_word,ndi.pos, ndi.df, "posting.txt", doc_score_list);
				//then we go through query, and count exactly how many times this_word appears. appear more = larger weight.
				int qtf = 0;
				for (int q_index = 0; q_index < query_list[q].words.size(); q_index++) {
					if (this_word.compare(query_list[q].words[q_index]) == 0) qtf++; //restricted to single words
				}
				for (int j = 0; j < doc_score_list.size(); j++) {

					doc_score_list[j].score = qtf*doc_score_list[j].score; //for words
					}
			}

			//we have a phrase instead
			//sum of qtf * how many words in it * how many times it occurs
			else if (this_word.find(" ",0) !=string::npos) {
				
				//Weight for a phrase is: Number of times it appears in the document * size of the phrase
				//The score for one phrase is: Number of times the phrase appears in the query * Weight of phrase
				//The sum of scores for all the phrases in the query is the final score. 
				
				int words_in_phrase = 0;
				vector<string> single_phrase_words;
				char * ptok;
				char anotherstring[256];
				strcpy(anotherstring,this_word.c_str()); //tokenize the phrase.
		 		ptok = strtok (anotherstring,"\" "); //one word in phrase
		 		while (ptok != NULL) {
					single_phrase_words.push_back(ptok);
					ptok = strtok (NULL, "\" ");
		 		}
				//pos here is for first word
				get_dpw_weight(single_phrase_words, "posting.txt", newdict_list, doc_score_phrases);

				int qtf = 0;
				for (int q_index = 0; q_index < query_list[q].words.size(); q_index++) {
					if (this_word.compare(query_list[q].words[q_index]) == 0) qtf++;
				}
				for (int j = 0; j < doc_score_phrases.size(); j++) doc_score_phrases[j].score = qtf*doc_score_phrases[j].score; //for phrases 
			}
		} //end of query

		//we should have a list of docids with corresponding word and phrase scores.
		for (int j = 0; j < doc_score_list.size(); j++) {
			int add_id = doc_score_list[j].docid;
			for (int b = 0; b < doc_score_phrases.size(); b++) {
				if (doc_score_phrases[b].docid == add_id) doc_score_list[j].score += doc_score_phrases[b].score;
			}
		}
		//now sort docids
		std::sort(doc_score_list.begin(), doc_score_list.end(), comp_doc_score);

//---------------------------------------------------- printing	


		printf("Query: %d being processed\n", q+1);
		fprintf(res,"Query: %d %s\n", q+1, query_unmod[q].c_str());

		//go through relevant documents list and find matching query id;
		struct rel_item one_rel;
		for(int i = 0; rel_list.size(); i++) {
			if (rel_list[i].query_id ==q+1) {
				one_rel = rel_list[i];
				break;
			}
		}
		
		//printer is just a counter that I keep reusing the name for.
		double rel_counter = 0;
		double sum_precision_at_rel = 0;
		for(int printer = 0; printer < 10; printer++) {
			int existing = 0;
			fprintf(res,"%d ",  doc_score_list[printer].docid);
			for (int r = 0; r < one_rel.doc_nums.size(); r++) {
				if (one_rel.doc_nums[r] == doc_score_list[printer].docid) {
					fprintf (res,"R\n");
					rel_counter++;
					sum_precision_at_rel += rel_counter / (printer+1); //add current precision at this point
					existing = 1;
				}
			}
			if (existing==0) fprintf (res,"N\n");
		}
		
		double recall = rel_counter / one_rel.doc_nums.size();
		double precision = rel_counter / 10;
		double ave_precision = sum_precision_at_rel / rel_counter; //sums of all precisions divided by number relevant
		fprintf(res,"For the top 10 results the recall = %f, Precision = %f, The MAP = %f\n", recall, precision, ave_precision);		

		final_recall += recall;
		final_precision += precision;
		final_mean += ave_precision;
		
		
		doc_score_list.clear();
		doc_score_phrases.clear();
	}
	
//recall: sum of recall / number of queries
// all the precisions / number of queries
//Map: all the old average precisions / number of queries.
	fprintf(res,"The average recall = %f, The average precision = %f, The MAP = %f\n", final_recall / (double) query_unmod.size(), final_precision / (double) query_unmod.size(), final_mean / (double) query_unmod.size());		
	return 0;
}
