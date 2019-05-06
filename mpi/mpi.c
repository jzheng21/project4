#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <stdint.h>
#include "sys/types.h"
#include "sys/sysinfo.h"

#define TAG_DONE_PRINTING					3
#define TAG_PRINT_SUBSTRINGS 			2
#define NUM_WIKI_LINES 						1000000

void algorithm(char **wiki_dump, char **longestCommonSubstring, int chunkSize);
void printData(char **longestCommonSubstring, int chunkSize, int rank);

typedef struct
{
	uint32_t virtualMem;
	uint32_t physicalMem;
} processMem_t;

int parseLine(char *line)
{
	int i = strlen(line);
	const char *p = line;
	while (*p < '0' || *p > '9') p++;
	line[i - 3] = '\0';
	i = atoi(p);
	return i;
}

/* Memory usage for both virtual and physical*/
void GetProcessMemory(processMem_t* processMem)
{
	FILE *file = fopen("/proc/self/status", "r");
	char line[128];

	while (fgets(line, 128, file) != NULL)
	{
		if (strncmp(line, "VmSize:", 7) == 0)
		{
			processMem->virtualMem = parseLine(line);
		}

		if (strncmp(line, "VmRSS:", 6) == 0)
		{
			processMem->physicalMem = parseLine(line);
		}
	}
	fclose(file);
}

int main(int argc, char *argv[])
{
	 double elapsedTime = 0.0;
	 struct timeval t1, t2;
	 processMem_t myMem;
	 long tag;
	 int linesToProcess = NUM_WIKI_LINES;
	 int rc, numTasks, rank, startIndex, endIndex, i, j, chunkSize;
	 MPI_Status Status;
	 MPI_Request *request = NULL;
	 FILE * fd = fopen("/homes/dan/625/wiki_dump.txt", "r");
	 char *buffer = NULL;
	 size_t n = 0;
	 char **longestCommonSubstring;

	 rc = MPI_Init(&argc, &argv);
	 if(rc != MPI_SUCCESS)
	 {
		 printf("Error starting MPI program. Terminating.\n");
		 fflush(stdout); // need to call fflush to force the printout, bc of MPI
		 MPI_Abort(MPI_COMM_WORLD, rc);
	 }

	 MPI_Comm_size(MPI_COMM_WORLD, &numTasks);
	 MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	 chunkSize = linesToProcess/numTasks;
	 startIndex = chunkSize * rank;
	 endIndex = (chunkSize * (rank + 1)) + 1; // + 1 to allocate the line for the last comparison, not last line used to compare
	 if(rank == (numTasks - 1)) // last rank
	 {
		 endIndex = linesToProcess;
	 }

	 j = 0;
	 int mallocSize = endIndex - startIndex;
	 char **wiki_dump = (char **) malloc(mallocSize * sizeof(char *));
	 for(i = 0; i < NUM_WIKI_LINES; i++)
	 {
		 size_t line_length = getline(&buffer, &n, fd);
		 if(i >= startIndex & i < endIndex)
		 {
		   wiki_dump[j] = (char*)malloc((line_length) * sizeof(char));
		   memcpy(wiki_dump[j], buffer, sizeof(char) * line_length);
		   wiki_dump[j][line_length-2] = 0;
			 j++;
	 	 }
	 }
	 fclose(fd);

	 longestCommonSubstring = (char **) malloc((mallocSize)* sizeof(char *));
	 gettimeofday(&t1, NULL);
	 algorithm(wiki_dump, longestCommonSubstring, chunkSize);

	 MPI_Barrier(MPI_COMM_WORLD);

	 if(rank == 0)
	 {
		 printData(longestCommonSubstring, chunkSize, rank);
		 tag = TAG_PRINT_SUBSTRINGS;
		 for(i = 1; i < numTasks; i++)
		 {
			 MPI_Send(&tag, 1, MPI_LONG, i, TAG_PRINT_SUBSTRINGS, MPI_COMM_WORLD);
			 MPI_Recv(&tag, 1, MPI_LONG, i, TAG_DONE_PRINTING, MPI_COMM_WORLD, &Status);
		 }
	 }
	 else
	 {
		 MPI_Recv(&tag, 1, MPI_LONG, 0, TAG_PRINT_SUBSTRINGS, MPI_COMM_WORLD, &Status);
		 printData(longestCommonSubstring, chunkSize, rank);
		 tag = TAG_DONE_PRINTING;
		 MPI_Send(&tag, 1, MPI_LONG, 0, TAG_DONE_PRINTING, MPI_COMM_WORLD);
	 }


	 gettimeofday(&t2, NULL);
	 GetProcessMemory(&myMem);
	 printf("Memory, Processes, %d, Rank, %d, vMem, %u KB, pMem, %u KB\n", numTasks, rank, myMem.virtualMem, myMem.physicalMem);
	 fflush(stdout);
	 free(longestCommonSubstring);
	 MPI_Finalize();

	 elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0; //sec to ms
 	 elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0; // us to ms

	 if(rank == 0)
	 {
 	 	 printf("DATA, %f, Processes, %d\n", elapsedTime, numTasks);
 	 }
	 return 0;
}

void algorithm(char **wiki_dump, char **longestCommonSubstring, int chunkSize)
{
	int p, k, i, j, s1_len, s2_len, col, val, max = 0;

	for(j = 0; j < chunkSize - 1; j++)
	{
		max = 0;
		char *s1 = wiki_dump[j];
		char *s2 = wiki_dump[j + 1];
		s1_len = strlen(wiki_dump[j]);
		s2_len = strlen(wiki_dump[j + 1]);

		int **table = (int **)malloc((s1_len + 1) * sizeof(int *));
		for(i = 0; i < s1_len+1; i++)
			table[i] = (int *)malloc((s2_len + 1) * sizeof(int));

		for(i = 0; i <= s1_len; i++)
			table[i][0] = 0;
		for(i = 0; i <= s2_len; i++)
			table[0][i] = 0;

		//set rest of table to correct values
		//get max value of table and store associated column
		for(i = 1; i <= s1_len; i++)
		{
			for(k = 1; k <= s2_len; k++)
			{
				if(s1[i-1] == s2[k-1])
				{
					val = table[i-1][k-1] + 1;
					table[i][k] = val;
					if(val > max)
					{
						max = val;
						col = k;
					}
				}
				else
					table[i][k] = 0;
			}
		}

		//Create longestCommonSubstring based on the max value from table
		if(max > 0)
		{
			char substr[max];
			for(i = 1; i <= max; i++)
			{
				substr[max-i] = s2[col-i];
			}
			longestCommonSubstring[j] = (char *) malloc((max + 1) * sizeof(char));
			memcpy(longestCommonSubstring[j], substr, sizeof(char) * max);
			longestCommonSubstring[j][max] = 0;
		}

		for(i = 0; i < s1_len; i++)
		{
			free(table[i]);
		}

		free(table);
	}
}

void printData(char **longestCommonSubstring, int chunkSize, int rank)
{
	int i;
	for(i = 0; longestCommonSubstring[i] != 0; i++) // i < NUM_WIKI_LINES - 1
	{
		printf("Lines%d-%d: ", i + (chunkSize*rank), (i+1) + (chunkSize*rank));
		if(longestCommonSubstring[i] != NULL)
		{
			printf("%s\n", longestCommonSubstring[i]);
		}
		else
		{
			printf("None found\n");
		}
	}
	fflush(stdout);

}
