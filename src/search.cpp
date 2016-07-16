#include<iostream>
#include<cstdio>
#include<cstring>
#include<string>
#include<vector>
#include<utility>
#include<algorithm>
#include<sstream>

using namespace std;

#define ALPHABET_SZ 26
#define BUFF_SZ 120000000
#define WORD_SZ 128

vector<string> Term;
vector<long long int> Offset;
vector<int> posting_list_len;
vector< pair<int, long long int> > id2t;

// LOAD SECONDARY INDEX CLASS -------------------------------------------------------------------------------------------

class LoadSecIdx
{
	public:
		FILE *fp;
		unsigned char* buff;
		int buffp;
		int buff_len;
		bool done;

		LoadSecIdx(char*);
		void Read();
		void BuffIdxInc();
		void Run();
		~LoadSecIdx();
};

LoadSecIdx::LoadSecIdx(char* fname)
{
	fp = fopen64(fname, "r");
	if(fp == NULL)
		puts("Error: Could not open file.");

	buff = new unsigned char[BUFF_SZ + 4];
	buffp = 0;
	buff_len = 0;
	done = false;
}

inline void LoadSecIdx::Read()
{
	buffp = 0;
	buff_len = fread(buff, 1, BUFF_SZ, fp);
	if(buff_len == 0)
		done = true;
}

inline void LoadSecIdx::BuffIdxInc()
{
	++buffp;
	if(buffp >= buff_len)
		Read();
}

void LoadSecIdx::Run()
{
    char word[WORD_SZ];

    Read();

    while(!done)
    {
        int i = 0;
        while(isalpha(buff[buffp]))
        {
            word[i++] = buff[buffp];
            BuffIdxInc();
        }
        word[i] = '\0';
        BuffIdxInc();

        long long off = 0;
        while(isdigit(buff[buffp]))
        {
            off = 10*off + (buff[buffp] - '0');
            BuffIdxInc();
        }
        BuffIdxInc();

        int l = 0;
        while(isdigit(buff[buffp]))
        {
            l = 10*l + (buff[buffp] - '0');
            BuffIdxInc();
        }
        BuffIdxInc();

        Term.push_back(string(word));
        Offset.push_back(off);
        posting_list_len.push_back(l);
    }
    Term.resize(Term.size());
    Offset.resize(Offset.size());
    posting_list_len.resize(posting_list_len.size());
}

LoadSecIdx::~LoadSecIdx()
{
	fclose(fp);
	delete[] buff;
}

// LOAD ID TO TITLE -------------------------------------------------------------------------------------------

class LoadId2Title
{
	public:
		FILE *fp;
		unsigned char* buff;
		int buffp;
		int buff_len;
		bool done;
        long long int offset;

		LoadId2Title(char*);
		void Read();
		void BuffIdxInc();
		void Run();
		~LoadId2Title();
};

LoadId2Title::LoadId2Title(char* fname)
{
	fp = fopen64(fname, "r");
	if(fp == NULL)
		puts("Error: Could not open file.");

	buff = new unsigned char[BUFF_SZ + 4];
	buffp = 0;
	buff_len = 0;
	done = false;
    offset = 0;
}

inline void LoadId2Title::Read()
{
	buffp = 0;
	buff_len = fread(buff, 1, BUFF_SZ, fp);
	if(buff_len == 0)
		done = true;
}

inline void LoadId2Title::BuffIdxInc()
{
    ++offset;
	++buffp;
	if(buffp >= buff_len)
		Read();
}

void LoadId2Title::Run()
{
    Read();

    while(!done)
    {
        int id = 0;
        while(isdigit(buff[buffp]))
        {
            id = 10*id + (buff[buffp] - '0');
            BuffIdxInc();
        }
        BuffIdxInc();

        id2t.push_back(make_pair(id, offset));

        int i = 0;
        while(buff[buffp] != '\n') BuffIdxInc();
        BuffIdxInc();
    }

    sort(id2t.begin(), id2t.end());
    id2t.resize(id2t.size());
}

LoadId2Title::~LoadId2Title()
{
	fclose(fp);
	delete[] buff;
}

int main(int argc, char* argv[])
{
	if(argc != 4)
	{
		puts("Usage: ./a.out <primary_idx> <sec_idx> <id2title>");
		return -1;
	}

    /*
    LoadSecIdx lhandler(argv[2]);
    lhandler.Run();

    LoadId2Title ihandler(argv[3]);
    ihandler.Run();
    */

    while(true)
    {
        string inp, t;
        vector<string> q;
        
        puts("\nEnter Query: ");
        getline(cin, inp);

        stringstream ss(inp);
        while(ss >> t)
            q.push_back(t);
    }

	return 0;
}
