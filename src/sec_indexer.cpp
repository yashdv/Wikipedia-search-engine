#include<iostream>
#include<cstdio>

using namespace std;

#define BUFF_SZ 120000000
#define WORD_SZ 128

// SECONDARY INDEX CLASS ---------------------------------------------------------------------------------------------------

class MakeSecIdx
{
	public:
		FILE *fp;
		unsigned char* buff;
		int buffp;
		int buff_len;
		long long int offset;
		bool done;

		MakeSecIdx(char*);
		void Read();
		void BuffIdxInc();
		int TraversePostingList();
		void Run();
		~MakeSecIdx();
};

MakeSecIdx::MakeSecIdx(char* fname)
{
	fp = fopen64(fname, "r");
	if(fp == NULL)
		puts("Error: Could not open file.");

	buff = new unsigned char[BUFF_SZ + 4];
	done = false;
	offset = 0;
	buffp = 0;
	buff_len = 0;
}

inline void MakeSecIdx::Read()
{
	buffp = 0;
	buff_len = fread(buff, 1, BUFF_SZ, fp);
	if(buff_len == 0)
		done = true;
}

inline void MakeSecIdx::BuffIdxInc()
{
	++offset;
	++buffp;
	if(buffp >= buff_len)
		Read();
}

inline int MakeSecIdx::TraversePostingList()
{
	int mask1 = 15;
	int mask2 = 15 << 4;
	char c;
	long long int curr_offset = offset;

	// two 10's will not appear in same byte
	do
	{
		// id
		for(int i=0; i<4; ++i)
		{
			c = buff[buffp];
			BuffIdxInc();
			if( (c & mask1)==10 || ((c & mask2) >> 4)==10 )
				break;
		}

		// freq
		while(true)
		{
			c = buff[buffp];
			BuffIdxInc();
			if( (c & mask1)==10 || ((c & mask2) >> 4)==10 )
				break;
		}

		// field and comma/newline
		c = buff[buffp];
		BuffIdxInc();
	}while(!(c & 1));

	return (offset - curr_offset);
}

void MakeSecIdx::Run()
{
	char term[WORD_SZ];
	int term_len;
	int len_curr_posting_list;
	long long int curr_offset;

	Read();
	while(true)
	{
		term_len = 0;
		while(isalpha(buff[buffp]))
		{
			term[term_len++] = buff[buffp];
			BuffIdxInc();
		}
		BuffIdxInc();
		term[term_len] = '\0';

		if(term_len == 0)
			break;

		curr_offset = offset;
		len_curr_posting_list = TraversePostingList();
        cout << term << " " << curr_offset << " " << len_curr_posting_list << endl;
	}
}

MakeSecIdx::~MakeSecIdx()
{
	fclose(fp);
	delete[] buff;
}

void BuildSecIdx(char* fname)
{
	MakeSecIdx handler(fname);
	if(handler.fp == NULL)
		return;
	handler.Run();
}

int main(int argc, char* argv[])
{
	if(argc != 2)
	{
		puts("Usage: ./a.out <primary_idx>");
		return -1;
	}
	BuildSecIdx(argv[1]);
	return 0;
}
