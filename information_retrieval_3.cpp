

#include <stdio.h>
#include <iostream>
#include <vector>

using namespace std;

struct dict_triple { //everything we need for the new dictionary
	char term[256];
	int doc_freq; 
	int term_freq; //not really part of the dictionary, but it's a convenient place to save this value
	int startbyte; //changed to use byte number
	int bytenum; //number of bytes until the next dictionary term.
};

//Very ugly byte encoding stuff
//encoding and decoding based on slides
vector<int> encode_num(int n) {
	vector<int> charbytes;
	int bytesize=0;
	while(true) {
		//mod n, stick it at front of vector. Calc current size
		//either break out or reset n and go again
		charbytes.insert(charbytes.begin(), n % 128); //add to beginning56
		bytesize++;
		if (n < 128) break;
		n = n/128;	
	}
	charbytes[bytesize-1] += 128;
	return charbytes; //an encoded number
}

vector<int> encode_num_list( vector<int> nums) { //list of numbers
	vector<int> bytes;
	vector<int> bytestream;

	for (int n = 0; n < nums.size(); n++) {
		bytes = encode_num(nums[n]);
		for (int m = 0; m < bytes.size(); m++) bytestream.push_back(bytes[m]) ; //everything goes in back
	}
	return bytestream;
}

//decode goes here.
//this is the same bytestream we returned from encoding
vector<int> decode_num_list( vector<int> bytestream ) {
	vector<int> nums;
	int n = 0;

	for(int i = 0; i < bytestream.size(); i++) {
		if (bytestream[i] < 128) 	{
			n = 128 * n + bytestream[i];
		}
		else {
			n = 128 * n + (bytestream[i] - 128);
			nums.push_back(n);
			n = 0;
		}

	}
	return nums;
}


int main (int argc, char* argv[]) {
	//there's 2 parts and I don't know how this is going to be tested, so optional specifiers

	string old_posting_str("");
	string old_dict_str("");
	string new_posting_str("");
	string new_dict_str("");
	string new_out_str("");

	if (argc == 1) {
	//no arguments: default part i
		old_posting_str.assign("test_posting.txt");
		old_dict_str.assign("test_dict.txt");
		new_posting_str.assign("newPosting.txt");
		new_dict_str.assign("newDict.txt");
		new_out_str.assign("printPosting.txt");
	}
	else if(strcmp(argv[1], "1")==0) {
	//part i
		old_posting_str.assign("test_posting.txt");
		old_dict_str.assign("test_dict.txt");
		new_posting_str.assign("newPosting.txt");
		new_dict_str.assign("newDict.txt");
		new_out_str.assign("printPosting.txt");
	}
	else if (strcmp(argv[1], "2")==0) {
	//part ii
		old_posting_str.assign("my_post.txt");
		old_dict_str.assign("my_new_dict.txt");
		new_posting_str.assign("NYTposting.txt");
		new_dict_str.assign("NYTdict.txt");
		new_out_str.assign("NYTprint.txt"); //not specified but I didn't feel like checking whether or not to print.
	}
	else {
	//incase I don't recognize the arguments, just do part i
		old_posting_str.assign("test_posting.txt");
		old_dict_str.assign("test_dict.txt");
		new_posting_str.assign("newPosting.txt");
		new_dict_str.assign("newDict.txt");
		new_out_str.assign("printPosting.txt");
	}


	//old dictionary, new dictionary output
	FILE * p_dict;
	FILE * p_new_dict;
	//old postings list, new list, pretty print list
	FILE * p_post;
	FILE * p_new_post;
	FILE * p_print;



	vector<dict_triple> new_dict;
	int dict_byte = 0;
	char dict_line_buf[256];
	char post_line_buf[256];

	p_dict = fopen (old_dict_str.c_str(),"rb");

	if ( p_dict == NULL)  {
		printf("ERROR OPENING DICTIONARY FILE\n");
		return 1;
	}
//-------------------------------------------------------------------------
//Make a dictionary
	while (feof(p_dict) == 0) {
		fgets(dict_line_buf,256,p_dict);
		if (feof(p_dict) != 0) break; //eof after retreiving? Since there's a newline at the end, break here.
		dict_triple dt;

		char * tempterm;
		int tempfreq = -1;
		int temppos = -1;

		//just reading from old file
		tempterm = strtok (dict_line_buf," "); //term
		tempfreq = atoi(strtok (NULL, " ")); //doc frequency
		temppos = atoi(strtok (NULL, " ")); //line in postings file

		
		string str_line_buf(dict_line_buf);
		strcpy(dt.term, 	str_line_buf.substr(0, str_line_buf.length() ).c_str()  );

		dt.term_freq = 0;

		dt.startbyte = 0; //where do we start? THIS is actually mostly what we care about in the dictionary.

		new_dict.push_back(dt);
	}
	fclose(p_dict);
//-------------------------------------------------------------------------

//okay! So now we have a dictionary new_dict, but not any byte values in yet. We'll fill those in now.


	int term_num = 0; //dictionary vector index
	
	p_post = fopen (old_posting_str.c_str(),"rb"); 
	if ( p_post == NULL)  {
		printf("ERROR OPENING POSTINGS FILE\n");
		return 1;
	}
	p_new_post = fopen (new_posting_str.c_str(),"wb"); 
	if ( p_new_post == NULL)  {
		printf("ERROR OPENING POSTINGS FILE\n");
		return 1;
	}

	char newest_term[256];
	char prev_term[256];
	int prev_docid = -1;
	
	//Gaps between documents and positions + where did the last record begin?
	int culm_doc_gap = 0;
	int culm_pos_gap = 0;
	int last_new = 0;

	vector<int> full_read;
	vector<int> full_encoded;
	
	while (feof(p_post) == 0) {
		fgets(post_line_buf,256,p_post); //get next line in postings
		if (feof(p_post) != 0) break; //did not realize my previous code printed a newline at the end of the file. Should have noticed.

		//SOME_TERM_HERE 21 474
		//always have 3 values
		char * tempterm; //term
		int tempid = -1; 
		int temppos = -1;
		tempterm = strtok (post_line_buf," "); //term
		strcpy(newest_term,tempterm);
		tempid = atoi(strtok (NULL, " ")); //doc id
		temppos = atoi(strtok (NULL, " ")); //pos within this document

		//so now I have the term itself, doc id number, and position within document, or one triple


		//same term, same docid. Just now we have more postings
		if (prev_docid == tempid && strcmp(prev_term,newest_term) == 0){		
			new_dict[term_num].startbyte = dict_byte; //shift pointer to current location
			new_dict[term_num].doc_freq++;

			new_dict[term_num].term_freq += 1;

			culm_doc_gap = 0;

			int pgap = temppos - culm_pos_gap;
			culm_pos_gap +=  temppos - culm_pos_gap;

			//edit last entry
			//how many blocks is this new entry?
			vector<int> block_counter;
			block_counter.push_back(temppos);
			block_counter = encode_num_list(block_counter);
			int blocks = block_counter.size();

			full_read.push_back(pgap); //add value only to end
			full_read[last_new]+=blocks; //find base bytes + blocks, add current blocks
			full_read[last_new+2] += 1; //termfreq

			dict_byte += blocks-1; //shift the pointer ahead.
		}


		//same term, DIFFERENT docid. We need a gap in this case.
		else if (prev_docid != tempid && strcmp(prev_term,newest_term) == 0) {
			new_dict[term_num].startbyte = dict_byte;
			new_dict[term_num].doc_freq++;
			new_dict[term_num].term_freq = 1; //new document

			//gap correlates to tempid
			//how far are we from previous document's docid 
			int gap = tempid - culm_doc_gap;
			culm_doc_gap += tempid - culm_doc_gap;

			culm_pos_gap = temppos;

			int base_bytes = 3; //size, doc number, tf for current. Add the number of bytes for pos onto this

			vector<int> block_counter;
			block_counter.push_back(temppos);
			block_counter = encode_num_list(block_counter);
			int blocks = block_counter.size();
			//num_bytes, doc_num, tf, pos_enc
			last_new = full_read.size();

			full_read.push_back(base_bytes+blocks);
			full_read.push_back(gap);//here is a gap!
			full_read.push_back(new_dict[term_num].term_freq);
			full_read.push_back(temppos);

			dict_byte+= base_bytes+blocks;
		}
		else {
		//new term and new document
			new_dict[term_num].startbyte = dict_byte; //current dictionary pointer location
			new_dict[term_num].doc_freq = 1;
			new_dict[term_num].term_freq = 1;

			culm_doc_gap = 0;
			culm_pos_gap = temppos;

			int base_bytes = 3; //size, doc number, tf for current. Add the number of bytes for pos onto this

			vector<int> block_counter;
			block_counter.push_back(temppos);
			block_counter = encode_num_list(block_counter);
			int blocks = block_counter.size();
			
			//num_bytes, doc_num, tf, pos_enc
			last_new = full_read.size();
			full_read.push_back(base_bytes+blocks);
			full_read.push_back(tempid);
			full_read.push_back(new_dict[term_num].term_freq);
			full_read.push_back(temppos);

			//count where we need to skip to.
			dict_byte += base_bytes+blocks;
			
			term_num++;
		}

		prev_docid = tempid;
		strcpy(prev_term,newest_term);
	}
	fclose(p_post);
	fclose(p_new_post);
	//all postings read

full_encoded = encode_num_list(full_read); //encode the entire list of original numbers

//DICTIONARY
	FILE * new_d;
	new_d=fopen (new_dict_str.c_str(),"w"); 
	for (int i = 0; i < new_dict.size(); i++ ){
		fprintf(new_d, "%s %d %d\n", new_dict[i].term, new_dict[i].doc_freq, new_dict[i].startbyte );
	}
	fclose(new_d);


//PRINT COMPRESSED POSTINGS TO FILE
	p_new_post = fopen (new_posting_str.c_str(),"wb");
	for (int i = 0; i < full_encoded.size();i++) {
		char to_write[1]; //unsigned? We already encoded so this should be okay.
		to_write[0] = full_encoded[i];
		fwrite(to_write, 1, 1, p_new_post); //write this byte to file.
	}
	fclose(p_new_post);

//PRETTY READ AND PRINT
	vector <int> fetched_encoded;
	p_new_post = fopen (new_posting_str.c_str(),"rb"); //reading
	for (int i = 0; i < full_encoded.size();i++) { //same number as original encoded vector! THIS IS IMPORTANT.
		unsigned char to_read[1];
		fread(to_read, 1, 1, p_new_post); //read one byte from file
		fetched_encoded.push_back(to_read[0]); //store in vector of fetched encoded bytes
	}
	fclose(p_new_post);

	vector<int> fetched_decoded = decode_num_list(fetched_encoded); //decode and store in new vector

	p_print = fopen (new_out_str.c_str(),"wb"); //writing
	for (int i = 0; i < fetched_decoded.size();) {
		int tf= 4 + fetched_decoded[i+2] - 1;
		for (int j = 0; j < 4+tf-1; j++) { //a record is at least 3, plus however many terms are in the term frequency. Weird math.
			fprintf(p_print, "%d ", fetched_decoded[i+j]);
			if (j == 2) tf = fetched_decoded[i+j]; //set term freq
		}
		fprintf(p_print, "\n");
		i+= 4+tf-1;
	}

	return 0;
}
