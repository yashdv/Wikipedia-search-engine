#include<iostream>
#include<cstdio>
#include<cctype>

using namespace std;

#define MAX 120000000

unsigned char buff[MAX];
char word[10010];

int main(int argc, char* argv[])
{
	if(argc != 2)
	{
		puts("Usage: ./a.out <bin_file_path>");
		return -1;
	}

	FILE *fp = fopen(argv[1], "r");
	int len = fread(buff, 1, MAX, fp);

	for(int i=0; i<len; ++i)
	{
		if(!isalpha(buff[i]))
			break;

        // read word
		int wi = 0;
		word[wi] = '\0';
		while(buff[i] != ' ')
		{
			word[wi] = buff[i];
			++wi;
			++i;
		}
		++i;
		word[wi] = '\0';
		if(*word != '\0')
			printf("%s ", word);

		do
		{

			int id = 0;
			int freq = 0;
			int inc = 0;
			int dig;
			int mask;
			int cnt = 0;

            // id
			while(true)
			{
				mask = (inc ? 15 : 15 << 4);
				dig = (buff[i] & mask) >> (inc ? 0 : 4);
				i += inc;
				inc = 1 - inc;
				if(dig == 10)
					break;
				id = 10*id + dig;
				++cnt;
				if(cnt == 8)
					break;
			}

            // freq
			while(true)
			{
				mask = (inc ? 15 : 15 << 4);
				dig = (buff[i] & mask) >> (inc ? 0 : 4);
				i += inc;
				inc = 1 - inc;
				if(dig == 10)
					break;
				freq = 10*freq + dig;
			}

			if(inc == 1)
				++i;

            // field
			mask = 31 << 3;
			int field = (buff[i] & mask) >> 3;

			printf("%d %d %d,", id, freq, field);

		} while(!(buff[i++] & 1));

		putchar('\n');
		--i;
	}

	fclose(fp);

	return 0;
}
