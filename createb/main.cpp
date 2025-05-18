#include <cstdio>
#include <cstdlib>
#include <sys/types.h>
#include <bits/localefwd.h>
#include "..//include//higplat.h"

void Usage(char* pszProgramName);

int main(int argc, char* argv[])
{
	char* lpname;
	uint size;
	lpname = NULL;
	size = 0;

	int i;
	for (i = 1; i < argc; i++)
	{
		if ((*argv[i] == '-') || (*argv[i] == '/'))
		{
			switch (tolower(*(argv[i] + 1)))
			{
			case 's':
				size = atoi(argv[++i]);
				break;
			case 'h':
			case '?':
			default:
				Usage(argv[0]);
				return 0;
			}
		}
		else
		{
			lpname = argv[i];
		}
	}

	if (lpname == NULL || size == 0)
	{
		Usage(argv[0]);
		return 0;
	}

	if (CreateB(lpname, size))
	{
		printf("Bulletin \'%s\' created successfully!\n", lpname);
		return 0;
	}
	printf("Create bulletin \'%s\' fail with error code %d\n", lpname, GetLastErrorQ());
	return 0;
}


void Usage(char* pszProgramName)
{
	fprintf(stderr, "Usage:  createb bulletin_name -s bulletin_size\n");
	fprintf(stderr, "Arguments:\n");
	fprintf(stderr,
		"\tbulletin_name\tname of the bulletin(max 20 characters)\n");
	fprintf(stderr,
		"\tbulletin_size\tbulletin size(in bytes)\n");
	fprintf(stderr,
		"Example:\tcreateb mybulletin -s 5000\n");

	exit(1);
}