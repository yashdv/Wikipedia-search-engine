#include<iostream>
#include<cstdio>
#include<cstring>
#include<cctype>
#include<dirent.h>
#include<queue>
#include<string>
#include<utility>

using namespace std;

#define BUFF_SZ 1200000
#define OBUFF_SZ 20000000

class MergeIndexes
{
	public:
		FILE** fp;
		char* path;
		unsigned char* obuff;
		unsigned char** buff;
		int* buff_len;
		int* buffp;
		int obuffp;
		int num_files;
		bool* done;
		priority_queue< pair<string, int>, vector< pair<string, int> >, greater< pair<string, int> > > heap;

		MergeIndexes(char*);
		int NumFiles();
		void OpenFiles();
		void Read(int);
		void BuffIdxInc(int);
		void PushTerm(int);
		void TraversePostingList(int);
		void Merge();
		~MergeIndexes();
};

MergeIndexes::MergeIndexes(char* path_)
{
	path = new char[128];
	strcpy(path, path_);
	num_files = NumFiles();

	fp = new FILE*[num_files];

	buff = new unsigned char*[num_files];
	for(int i=0; i<num_files; ++i)
		buff[i] = new unsigned char[BUFF_SZ + 4];

	obuff = new unsigned char[OBUFF_SZ];
	buffp = new int[num_files];
	buff_len = new int[num_files];
	done = new bool[num_files];
}

int MergeIndexes::NumFiles()
{
	DIR* dir = opendir(path);
	if(dir == NULL)
	{
		puts("Error: Cant open directory");
		return 0;
	}

	struct dirent* file;
	int cnt = 0;

	while((file = readdir(dir)) != NULL)
		if(file->d_name[0] != '.') ++cnt;

	closedir(dir);
	return cnt;
}

void MergeIndexes::OpenFiles()
{
	DIR* dir = opendir(path);
	struct dirent* file;
	int path_len = strlen(path);
	int fname_len;
	int fp_idx = 0;

	path[path_len] = '/';
	while((file = readdir(dir)) != NULL)
	{
		if(file->d_name[0] == '.')
			continue;
		fname_len = strlen(file->d_name);

		for(int i=0; i<fname_len; ++i)
			path[path_len + 1 + i] = file->d_name[i];
		path[path_len + 1 + fname_len] = '\0';

		fp[fp_idx++] = fopen(path, "r");
	}

	path[path_len] = '\0';
	closedir(dir);
}

inline void MergeIndexes::Read(int idx)
{
	buffp[idx] = 0;
	buff_len[idx] = fread(buff[idx], 1, BUFF_SZ, fp[idx]);
	if(buff_len[idx] == 0)
		done[idx] = true;
}

inline void MergeIndexes::BuffIdxInc(int idx)
{
	++buffp[idx];
	if(buffp[idx] >= buff_len[idx])
		Read(idx);
}

inline void MergeIndexes::PushTerm(int idx)
{
	if(done[idx])
		return;

	char c;
	string term;

	while(isalpha((c = buff[idx][buffp[idx]])))
	{
		BuffIdxInc(idx);
		term += c;
	}
	BuffIdxInc(idx);

	if(!term.empty())
		heap.push(make_pair(term, idx));
}

void MergeIndexes::TraversePostingList(int idx)
{
	int mask1 = 15;
	int mask2 = 15 << 4;
	char c;
	bool end_of_posting_list = false;

	obuffp = 0;
	// two 10's will not appear in same byte
	do
	{
		// id
		for(int i=0; i<4; ++i)
		{
			c = buff[idx][buffp[idx]];
			obuff[obuffp++] = c;
			BuffIdxInc(idx);
			if( (c & mask1)==10 || ((c & mask2) >> 4)==10 )
				break;
		}

		// freq
		while(true)
		{
			c = buff[idx][buffp[idx]];
			obuff[obuffp++] = c;
			BuffIdxInc(idx);
			if( (c & mask1)==10 || ((c & mask2) >> 4)==10 )
				break;
		}

		// field and comma/newline
		c = buff[idx][buffp[idx]];
		end_of_posting_list = (c & 1);
		obuff[obuffp++] = c;
		BuffIdxInc(idx);

	}while(!end_of_posting_list);

	obuff[obuffp - 1] &= ((1 << 8) - 2);
}

void MergeIndexes::Merge()
{
	string term;
	string curr_term = "";
	int idx;

	for(int i=0; i<num_files; ++i)
	{
		done[i] = false;
		Read(i);
		PushTerm(i);
	}

	obuffp = 0;
	while(!heap.empty())
	{
		term = heap.top().first;
		idx = heap.top().second;
		heap.pop();

		if(term != curr_term)
		{
			if(obuffp != 0)
			{
				obuff[obuffp - 1] |= 1;
				fwrite(obuff, 1, obuffp, stdout);
			}

			curr_term = term;
			string s = curr_term + " ";
			fwrite(s.c_str(), 1, s.size(), stdout);
		}
		else
		{
			fwrite(obuff, 1, obuffp, stdout);
		}

		TraversePostingList(idx);
		PushTerm(idx);
	}

	if(obuffp != 0)
	{
		obuff[obuffp - 1] |= 1;
		fwrite(obuff, 1, obuffp, stdout);
	}
}

MergeIndexes::~MergeIndexes()
{
	delete[] path;

	for(int i=0; i<num_files; ++i)
		fclose(fp[i]);
	delete[] fp;

	for(int i=0; i<num_files; ++i)
		delete[] buff[i];
	delete[] buff;

	delete[] obuff;
	delete[] buffp;
	delete[] buff_len;
	delete[] done;
}

int main(int argc, char* argv[])
{
	if(argc != 2)
	{
		puts("Usage: ./a.out <partial_idx_directory");
		return -1;
	}

	MergeIndexes handler(argv[1]);
	if(handler.num_files != 0)
	{
		handler.OpenFiles();
		handler.Merge();
	}
	return 0;
}
