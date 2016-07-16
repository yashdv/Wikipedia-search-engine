#include<iostream>
#include<cstdio>
#include<cstdlib>
#include<cstring>
#include<dirent.h>

using namespace std;

#define MAX 120000000

class ID2Title
{
    public:
        FILE* fp;
        char* buff;
		int file_size; // fits in int as files are in chunks

        ID2Title();
        bool OpenFile(char*);
		void ReadFile();
		void Parse();
        ~ID2Title();
};

ID2Title::ID2Title()
{
    buff = new char[MAX];
}

bool ID2Title::OpenFile(char* filename)
{
	fp = fopen(filename, "r");
	return (fp != NULL);
}

void ID2Title::ReadFile()
{
	file_size = fread(buff, 1, MAX, fp);
	buff[file_size] = '\0';
	fclose(fp);
}

void ID2Title::Parse()
{
	int title_start_idx;
    int title_len;
	bool page_started = false;
    int id_start_idx;
    int id_len;

	for(int i=0; i<file_size; ++i)
	{
		if(buff[i] == '<' && strncmp(&buff[i], "<page", 5) == 0)
		{
			page_started = true;
			title_start_idx = -1;
			id_start_idx = -1;
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
                title_len = 0;
				while(buff[i] != '<' && strncmp(&buff[i], "</title>", 8) != 0)
                {
                    ++i;
                    ++title_len;
                }
				i += 7; // 8 = 7 + (one ++ is done by the for loop)
			}
			else if(strncmp(&buff[i], "<id", 3) == 0 && id_start_idx == -1)
			{
				i += 3;
				while(buff[i] != '>') ++i;
				++i;

				id_start_idx = i;
                id_len = 0;
				while(isdigit(buff[i]))
				{
					++i;
                    ++id_len;
				}
				i += 4; // 5 = 4 + (one ++ is done by the for loop)

                fwrite(&buff[id_start_idx], 1, id_len, stdout);
                fwrite(" ", 1, 1, stdout);
                fwrite(&buff[title_start_idx], 1, title_len, stdout);
                fwrite("\n", 1, 1, stdout);
			}
		}
	}
}

ID2Title::~ID2Title()
{
	delete[] buff;
}

void Mapid2title(char* path)
{
	DIR* dir = opendir(path);
	if(dir == NULL)
	{
		puts("Error: Cant open directory");
		return;
	}

    int path_len = strlen(path);
    struct dirent* file;
    ID2Title handler;

    path[path_len] = '/';

    while((file = readdir(dir)) != NULL)
    {
        if(file->d_name[0] == '.')
            continue;

        int fname_len = strlen(file->d_name);

		for(int i=0; i<fname_len; ++i)
			path[path_len + 1 + i] = file->d_name[i];
		path[path_len + 1 + fname_len] = '\0';

        if(!handler.OpenFile(path))
        {
            printf("Could not open file: %s\n", path);
            return;
        }

        handler.ReadFile();
        handler.Parse();
    }

    closedir(dir);
}

int main(int argc, char *argv[])
{
    if(argc != 2)
    {
        puts("Usage: ./a.out <corpus dir>");
        exit(-1);
    }

    char path[256];
    strcpy(path, argv[1]);
	Mapid2title(path);

	return 0;
}
