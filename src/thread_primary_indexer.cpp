#include<iostream>
#include<cstdio>
#include<cstdlib>
#include<cstring>
#include<cctype>
#include<algorithm>
#include<map>
#include<set>
#include<string>
#include<vector>
#include<dirent.h>
#include<pthread.h>

#include "stemmer/porter_stemmer_thread_safe.hpp"

using namespace std;

#define tr(c,i) for(typeof((c).begin()) i = (c).begin(); i != (c).end(); i++)
#define MAX 120000000
#define CHUNK_SZ 90000000
#define WRD_SZ 10010
#define MAX_WRD_SZ 100
#define ASCII_SZ 128
#define ALPHABET_SZ 26
#define NUM_THREADS 4

// STOP WORDS -----------------------------------------------------------------------------------------------
char stop_words_list[][32] = {
	"able","about","above","abst","accordance","according","accordingly","across","act","actually","added","adj","affected","affecting","affects","after","afterwards","again","against","ah","all",
	"almost","alone","along","already","also","although","always","am","among","amongst","an","and","another","any","anybody","anyhow","anymore","anyone","anything","anyway","anyways","anywhere",
	"apparently","approximately","are","aren","arent","arise","around","as","aside","ask","asking","at","available","away","awfully","back","be","became","because","become","becomes","becoming",
	"been","before","beforehand","begin","beginning","beginnings","begins","behind","being","believe","below","beside","besides","between","beyond","biol","both","brief","briefly","but","by",
	"ca","came","can","cannot","cause","causes","co","com","come","comes","could","couldnt","did","different","do","does","doing","done","down","downwards","due","during","each","ed","edu",
	"effect","eg","either","else","elsewhere","end","ending","enough","especially","et","etc","even","ever","every","everybody","everyone","everything","everywhere","ex","except","far","few",
	"ff","fix","followed","following","follows","for","former","formerly","forth","found","from","further","furthermore","gave","get","gets","getting","give","given","gives","giving","go","goes",
	"gone","got","gotten","had","happens","hardly","has","have","having","he","hed","hence","her","here","hereafter","hereby","herein","heres","hereupon","hers","herself","hes","hi","hid","him",
	"himself","his","hither","home","how","howbeit","however","hundred","id","ie","if","im","immediate","immediately","importance","important","in","indeed","instead","into","is","it","itd","its",
	"itself","just","keep","keeps","kept","know","known","knows","largely","last","lately","later","latter","latterly","least","less","lest","let","lets","like","liked","likely","line",
	"little","look","looking","looks","made","mainly","make","makes","many","may","maybe","me","meantime","meanwhile","merely","mg","might","miss","ml","more","moreover","most","mostly","mr",
	"mrs","much","must","my","myself","na","nay","nd","near","nearly","necessarily","necessary","need","needs","neither","never","nevertheless","new","no","nobody","non","none","nonetheless",
	"noone","nor","normally","nos","not","noted","nothing","now","nowhere","obtain","obtained","obviously","of","off","often","oh","ok","okay","old","omitted","on","only","onto","or","ord","other",
	"others","otherwise","ought","our","ours","ourselves","out","outside","over","overall","owing","own","part","particular","particularly","per","perhaps","placed","please","poorly","possible",
	"possibly","potentially","pp","present","previously","primarily","probably","promptly","provides","put","que","quickly","quot","qv","ran","rather","rd","re","readily","really","recent","recently",
	"ref","refs","regarding","regardless","regards","related","relatively","respectively","resulted","resulting","results","run","said","same","saw","say","saying","says","sec","see","seeing","seem",
	"seemed","seeming","seems","seen","self","selves","sent","several","shall","she","shes","should","show","showed","shown","showns","shows","significant","significantly","similar","similarly",
	"since","slightly","so","some","somebody","somehow","someone","somethan","something","sometime","sometimes","somewhat","somewhere","soon","sorry","specifically","specified","specify",
	"specifying","stop","strongly","sub","substantially","successfully","such","sufficiently","suggest","sup","sure","take","taken","taking","tell","th","than","thank","thanks","that","thats",
	"the","their","theirs","them","themselves","then","thence","there","thereafter","thereby","thered","therefore","therein","thereof","therere","theres","thereto","thereupon","these","they",
	"theyd","theyre","think","this","those","thou","though","thoughh","thousand","throug","through","throughout","thru","thus","til","tip","to","together","too","took","toward","towards","tried",
	"tries","truly","try","trying","ts","un","under","unfortunately","unless","unlike","unlikely","until","unto","up","upon","ups","us","use","used","useful","usefully","usefulness","uses",
	"using","usually","value","various","very","via","viz","vol","vols","vs","want","wants","was","way","we","wed","welcome","went","were","what","whatever","whats","when","whence","whenever",
	"where","whereafter","whereas","whereby","wherein","wheres","whereupon","wherever","whether","which","while","whim","whither","who","whod","whoever","whole","whom","whomever","whos","whose",
	"why","widely","willing","wish","with","within","without","words","would","www","yes","yet","you","youd","your","youre","yours","yourself","yourselves"
};

int num_stop_words = 557;

char** stop_words[ASCII_SZ]; // sorted list of stop words indexed by alphabets
int word_count[ASCII_SZ];

inline void FillStopWords()
{
	for(int i=0; i<num_stop_words; ++i)
		++word_count[stop_words_list[i][0]];

	for(int i=0; i<ASCII_SZ; ++i)
		stop_words[i] = new char* [word_count[i]];

	int cnt = 0;
	for(int i='a'; i<='z'; ++i)
		for(int j=0; j<word_count[i]; ++j)
			stop_words[stop_words_list[cnt][0]][j] = stop_words_list[cnt], ++cnt;
}

inline void FreeStopWords()
{
	for(int i=0; i<ASCII_SZ; ++i)
		if(word_count[i])
			delete[] stop_words[i];
}

inline bool cmp(const char* x, const char* y)
{
	return (strcmp(x, y) < 0);
}

inline bool IsStopWord(char* word)
{
	return binary_search(
			stop_words[*word],
			stop_words[*word] + word_count[*word],
			word,
			cmp);
}

// POSTING LIST -----------------------------------------------------------------------------------------------

class DocData
{
	public:
		int id;
		int freq;
		int fields;

		DocData(int, int);
};

DocData::DocData(int id_, int fields_)
{
	id = id_;
	freq = 1;
	fields = fields_;
}

// TRIE NODE -----------------------------------------------------------------------------------------------

class TrieNode
{
	public:
		vector<DocData> docs;
		TrieNode* children[ALPHABET_SZ];

		TrieNode();
};

TrieNode::TrieNode()
{
	for(int i=0; i<ALPHABET_SZ; ++i)
		children[i] = NULL;
}

// TRIE -----------------------------------------------------------------------------------------------

class Trie
{
	public:
		TrieNode* root;

		Trie();
		void Insert(char*, int, int, int);
		void Clear(TrieNode* );
		~Trie();
};

Trie::Trie()
{
	root = new TrieNode;
}

inline void Trie::Insert(char* word, int word_len, int page_id, int field)
{
	TrieNode* p = root;
	int child_idx;

	for(int level=0; level<word_len; ++level)
	{
		child_idx = word[level] - 'a';
		if(p->children[child_idx] == NULL)
			p->children[child_idx] = new TrieNode;
		p = p->children[child_idx];
	}

	if(!p->docs.empty() && p->docs.back().id == page_id)
	{
		p->docs.back().freq += 1;
		p->docs.back().fields |= field;
	}
	else
	{
		DocData d(page_id, field);
		p->docs.push_back(d);
	}
}

void Trie::Clear(TrieNode* p)
{
	for(int i=0; i<ALPHABET_SZ; ++i)
		if(p->children[i] != NULL)
			Clear(p->children[i]);
	delete p;
}

Trie::~Trie()
{
	Clear(root);
}

// PARSER AND INVERTED INDEX -----------------------------------------------------------------------------------
class XMLParser
{
	public:
		FILE* out_fp;
		char* buff;
		char* word;
		int file_size; // fits in int as files are in chunks, LLI in merge ??
		int curr_page_id; // 8 digits max
		int field; // t b i l c
		Trie trie;
		struct stemmer* z; // porter stemming

		enum {TITLE = 16, BODY = 8, IBOX = 4, LINKS = 2, CATG = 1};

		XMLParser();
		~XMLParser();
		bool OpenOutFile(char*);
		bool ReadFile(char*);
		void RecordWord(int);
		void ParseTitle(char*);
		void ParseBody(char*);
		void Parse();
		int CompressDocData(vector<DocData>::iterator, unsigned char*, int*, bool);
		void PrintInvIdx(TrieNode*, int);
};

XMLParser::XMLParser()
{
	file_size = 0;
	z = create_stemmer();
	buff = new char[MAX];
	word = new char[WRD_SZ];
}

XMLParser::~XMLParser()
{
	free_stemmer(z);
	delete[] buff;
	delete[] word;
}

bool XMLParser::OpenOutFile(char* filename)
{
	out_fp = fopen(filename, "w");
	return (out_fp != NULL);
}

bool XMLParser::ReadFile(char* filename)
{
	FILE* fp = fopen(filename, "r");
	if(fp == NULL)
		return false;

	file_size = fread(buff, 1, MAX, fp);
	buff[file_size] = '\0';

	fclose(fp);	
	return true;
}

inline void XMLParser::RecordWord(int word_len)
{
	if(word_len < 3 || word_len >= MAX_WRD_SZ)
		return;

	if(!IsStopWord(word))
	{
		word_len = stem(z, word, word_len-1) + 1;
		word[word_len] = '\0';

		if(!IsStopWord(word))
			trie.Insert(word, word_len, curr_page_id, field);
	}
}

void XMLParser::ParseTitle(char *it)
{
	char endtag[] = "</title>";
	int endtag_len = strlen(endtag);
	int word_len;

	for(; ; ++it)
	{
		word_len = 0;
		*it = tolower(*it);
		while(isalpha(*it))
		{
			if(word_len < MAX_WRD_SZ)
				word[word_len] = *it;
			++word_len;
			++it;
			*it = tolower(*it);
		}
		if(word_len < MAX_WRD_SZ)
			word[word_len] = '\0';

		RecordWord(word_len);

		if(*it == '<' && strncmp(it, endtag, endtag_len) == 0)
			break;
	}
}

void XMLParser::ParseBody(char* it)
{
	char endtag[] = "</text>";
	int endtag_len = strlen(endtag);
	int word_len;
	int ibox_stack_cnt;
	int ext_link_cnt;
	bool double_eq = false;
	bool external_link_section = false;
	bool star_started = false;

	for(; ; ++it)
	{
		// text end tag check
		if(*it == '<' && strncmp(it, endtag, endtag_len) == 0)
			break;

		if(*it == '{' && *(it+1) == '{')
		{
			// infobox
			it += 2;
			if(field != IBOX)
			{
				while(isspace(*it)) ++it;
				if(strncmp(it, "Infobox", 7) == 0)
				{
					field = IBOX;
					it += 7;
					ibox_stack_cnt = 1;
				}
			}
			else
				++ibox_stack_cnt;
			--it; // ++it in for loop
		}
		else if(field == IBOX && *it == '}' && *(it+1) == '}')
		{
			++it;
			--ibox_stack_cnt;
			if(ibox_stack_cnt == 0)
				field = BODY;
		}
		else if(*it == '[' && *(it+1) == '[')
		{
			// category
			it += 2;
			if(strncmp(it, "Category:", 9) == 0)
			{
				field = CATG;
				it += 9;
			}
			--it; // ++it in for loop
		}
		else if(field == CATG && *it == ']' && *(it+1) == ']')
		{
			++it;
			field = BODY;
		}
		else if(*it == '=' && *(it+1) == '=')
		{
			// external links title
			while(*it == '=') ++it;
			--it; // ++it in for loop
			ext_link_cnt = 0;
			double_eq = !double_eq;
		}
		else if(external_link_section && !double_eq)
		{
			// external links
			if(!star_started)
			{
				if(*it == '*')
				{
					star_started = true;
					field = LINKS;
				}
			}
			else
			{
				if(*it == '\n' && *(it+1) != '*')
				{
					star_started = false;
					external_link_section = false;
					field = BODY;
				}
			}
		}

		// get word
		*it = tolower(*it);
		if(isalpha(*it))
		{
			word_len = 0;
			while(isalpha(*it))
			{
				if(word_len < MAX_WRD_SZ)
					word[word_len] = *it;
				++word_len;
				++it;
				*it = tolower(*it);
			}
			if(word_len < MAX_WRD_SZ)
				word[word_len] = '\0';
			--it;

			if(double_eq)
			{
				if(ext_link_cnt == 0)
				{
					if(strcmp(word, "external") == 0)
						++ext_link_cnt;
				}
				else if(ext_link_cnt == 1)
				{
					if(strcmp(word, "links") == 0)
						if(ext_link_cnt == 1)
							external_link_section = true;
					++ext_link_cnt;
				}
			}

			RecordWord(word_len);
		}
	}
}

void XMLParser::Parse()
{
	int title_start_idx;
	bool page_started = false;

	for(int i=0; i<file_size; ++i)
	{
		if(buff[i] == '<' && strncmp(&buff[i], "<page", 5) == 0)
		{
			page_started = true;
			title_start_idx = -1;
			curr_page_id = -1;
			i += 5;
			while(buff[i] != '>') ++i;
			++i;
		}

		if(page_started && buff[i] == '<')
		{
			if(strncmp(&buff[i], "</page>", 7) == 0)
			{
				page_started = false;
				i += 6; // 7 = 6 + (one ++ is done by the for loop)
			}
			else if(strncmp(&buff[i], "<title", 6) == 0)
			{
				i += 6;
				while(buff[i] != '>') ++i;
				++i;

				title_start_idx = i;
				while(buff[i] != '<' && strncmp(&buff[i], "</title>", 8) != 0) ++i;
				i += 7; // 8 = 7 + (one ++ is done by the for loop)
			}
			else if(strncmp(&buff[i], "<id", 3) == 0 && curr_page_id == -1)
			{
				i += 3;
				while(buff[i] != '>') ++i;
				++i;

				curr_page_id = 0;
				while(isdigit(buff[i]))
				{
					curr_page_id = 10*curr_page_id + (buff[i] - '0');
					++i;
				}
				i += 4; // 5 = 4 + (one ++ is done by the for loop)
			}
			else if(strncmp(&buff[i], "<text", 5) == 0)
			{
				i += 5;
				while(buff[i] != '>') ++i;
				++i;

				field = TITLE;
				ParseTitle(&buff[title_start_idx]);
				field = 0;

				field = BODY;
				ParseBody(&buff[i]);
				field = 0;
				i += 6; // 7 = 6 + (one ++ is done by the for loop)
			}
		}
	}
}

inline int XMLParser::CompressDocData(vector<DocData>::iterator doc, unsigned char* encoded_doc_data, int* temp, bool newline)
{
	int p = 0;
	int p_incrementor = 0;
	int len = 0;

	// id
	for(int x=doc->id; x>0; ++len, x/=10)
		temp[len] = x % 10;

	for(int i=len-1; i>=0; --i)
	{
		encoded_doc_data[p] |= (temp[i] << (4*(1-p_incrementor)));
		p += p_incrementor;
		p_incrementor = 1 - p_incrementor;
	}

	if(len != 8)
	{
		encoded_doc_data[p] |= (10 << (4*(1-p_incrementor)));
		p += p_incrementor;
		p_incrementor = 1 - p_incrementor;
	}

	// freq
	len = 0;
	for(int x=doc->freq; x>0; ++len, x/=10)
		temp[len] = x % 10;

	for(int i=len-1; i>=0; i--)
	{
		encoded_doc_data[p] |= (temp[i] << (4*(1-p_incrementor)));
		p += p_incrementor;
		p_incrementor = 1 - p_incrementor;
	}

	encoded_doc_data[p] |= (10 << (4*(1-p_incrementor)));
	p += p_incrementor;
	p_incrementor = 1 - p_incrementor;

	// fields
	if(p_incrementor == 1)
		++p;

	encoded_doc_data[p] |= (doc->fields << 3);
	encoded_doc_data[p] |= (newline ? 1 : 0);
	++p;
	return p;
}

void XMLParser::PrintInvIdx(TrieNode* p, int lev)
{
	++lev;
	if(!p->docs.empty())
	{
		word[lev] = ' ';
		fwrite(word, 1, lev+1, out_fp);

		unsigned char encoded_doc_data[16];
		int temp[8];
		int len;

		tr(p->docs, it)
		{
			memset(encoded_doc_data, '\0', sizeof encoded_doc_data);
			len = CompressDocData(it, encoded_doc_data, temp, (it+1 == p->docs.end()));
			fwrite(encoded_doc_data, 1, len, out_fp);
			//printf("%d %d %d,", it->id, it->freq, it->fields);
		}
		//putchar('\n');
	}

	for(int i=0; i<ALPHABET_SZ; ++i)
	{
		if(p->children[i] != NULL)
		{
			word[lev] = 'a' + i;
			PrintInvIdx(p->children[i], lev);
		}
	}
}

char* xml_path;
char* idx_path;

void* MakePartialIndex(void* arg)
{
	char* fname = (char*)arg;
	char temp1[128];
	char temp2[128];
	XMLParser handler;

	sprintf(temp1, "%s/%s", xml_path, fname);
	sprintf(temp2, "%s/i%s", idx_path, fname);

	if(!handler.ReadFile(temp1))
		printf("Could not open xml file: %s\n", fname);
	else
	{
		if(!handler.OpenOutFile(temp2))
			printf("Could not open idx file: i%s\n", fname);
		else
		{
			handler.Parse();
			handler.PrintInvIdx(handler.trie.root, -1);
		}
	}
}

int main(int argc, char *argv[])
{
	if(argc != 3)
	{
		puts("Usage: ./a.out <xml_path> <idx_path>");
		exit(-1);
	}
	xml_path = argv[1];
	idx_path = argv[2];

	DIR* dir = opendir(argv[1]);
	if(dir == NULL)
	{
		printf("Error in opening directory %s\n", argv[1]);
		exit(-1);
	}

	FillStopWords();

	pthread_t threads[NUM_THREADS];
	pthread_attr_t attr;
	int thread_cnt = 0;
	int cnt = 0;
	struct dirent* file;
	int rc;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	while((file = readdir(dir)) != NULL)
	{
		if(file->d_name[0] == '.')
			continue;

		puts(file->d_name);
		if((rc = pthread_create(&threads[thread_cnt], NULL, MakePartialIndex, (void*) file->d_name)))
			printf("Error in pthread_create(): %d\n", rc);
		++thread_cnt;
		++cnt;

		if(thread_cnt == NUM_THREADS)
		{
			for(int i=0; i<thread_cnt; ++i)
				if((rc = pthread_join(threads[i], NULL)))
					printf("Error in pthread_join(): %d\n", rc);
			thread_cnt = 0;
			printf("%d\n", cnt);
		}
	}

	if(thread_cnt != 0)
	{
			for(int i=0; i<thread_cnt; ++i)
				if((rc = pthread_join(threads[i], NULL)))
					printf("Error in pthread_join(): %d\n", rc);
			printf("%d\n", cnt);
	}

	closedir(dir);
	pthread_exit(NULL);
	FreeStopWords();
	return 0;
}
