/*
    Compilation:
        Use the command 'gcc -O3 -o substring_search substring_search.c' to compile the program

    Usage:
        Run the program with the command './substring_search'
    
    Note:
        This program does not serve a useful purpose, it was simply something I did for fun to see how fast I could get it to run. It is purely focused
        on speed. And it seems pretty fast!

        This code also contains a lot of comments, I did this because I want to know why I did something if I ever come back to it. It is for me to learn,
        and to easily remember what I learned when I come back to it

    Description:
        The provided C program is designed to efficiently search for a specific substring within a larger text string contained in a file. It employs the
        Boyer-Moore string search algorithm, a technique known for skipping non-matching sections of text to optimize search performance. The program reads
        data from a specified text file in chunks, and each chunk is then processed in a separate thread. This multi-threading approach enhances the speed of
        the program by allowing concurrent execution of the search function across different chunks of the text. If a match is found, the program reports the
        time taken to complete the search

    Performance Metrics:
        - The program can search over 112,386,000 characters for a substring at the very end in just 0.003-0.004 seconds
*/
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <windows.h> // For Memory Mapped Files
#include <process.h> // For multithreading

#define UNIQUE_CHARS 256 // ASCII's unique character count
#define MAX_THREADS 16   // Maximum number of threads for parallel processing

/*
    This structure is used to pass arguments to the search function that is executed by multiple threads. It
    encapsulates all the information needed to perform the Boyer-Moore string search algorithm on a chunk of the text
*/
typedef struct {
    char *text;
    int   textLength;
    char *pattern;
    int   patternLength;
    int   badCharShiftTable[UNIQUE_CHARS];
    int   found_at;
} SearchArgs;

/*
    The search function is the core of the Boyer-Moore string search algorithm. It's designed to be run as a thread, 
    and thus takes in a pointer to a SearchArgs struct, which contains the necessary data for the search operation

    Inputs:
        args - A pointer to a SearchArgs structure which contains the necessary data for the search operation

    Outputs:
        Returns the location in the text where the pattern was found, or -1 if not found

    Usage Example:
        This function is not called directly, but rather via a thread creation function such as _beginthreadex
*/
unsigned __stdcall search(void *args)
{
    /*
        First, we typecast the void pointer to a SearchArgs pointer because the args pointer passed to the thread function is always a void pointer. 
        However, in this context, we know the actual data type is SearchArgs
    */
    SearchArgs *searchArgs = (SearchArgs *)args;

    /* 
        Next, we extract the necessary fields from the searchArgs. These are:
            1. The 'text' and its length, which is the larger string in which we're searching for the pattern
            2. The 'pattern' and its length, which is the string we are trying to find within the larger text
            3. The 'badCharShiftTable', a precomputed table that's specific to the pattern and crucial for the Boyer-Moore search algorithm
                - This table allows the algorithm to skip ahead in the text after mismatches, thus enhancing efficiency
    */
    char *text          = searchArgs->text;
    int   textLength    = searchArgs->textLength;
    char *pattern       = searchArgs->pattern;
    int   patternLength = searchArgs->patternLength;
    int  *badCharShiftTable = searchArgs->badCharShiftTable;

    int patternShift = 0;

    /* 
        The outer while loop continues until the pattern cannot fit into the remaining part of the text. This ensures we
        don't check a pattern against a portion of the text that is smaller than the pattern itself
    */
    while (patternShift <= (textLength - patternLength))
    {
        int patternIndex = patternLength - 1;

        /* 
            The inner while loop compares characters from the end of the pattern to the corresponding characters in the text.
            This is consistent with the Boyer-Moore algorithm's approach of starting checks from the end of the pattern
        */
        while (patternIndex >= 0 && pattern[patternIndex] == text[patternShift + patternIndex])
        {
            patternIndex--;
        }

        /*
            If patternIndex is less than zero, it means the pattern has been found in the text at the current shift position.
            In this case, we record the location of the found pattern and return from the function as the search was successful
        */
        if (patternIndex < 0)
        {
            searchArgs->found_at = patternShift;
            return 0;
        }

        /* 
            If the pattern wasn't found, we shift it to align it with the next possible character in the text. We calculate this
            shift by using the badCharShiftTable and patternIndex. The maximum function ensures we move at least one position in
            the text, even if the table suggests a shift of zero
        */
        patternShift += max(1, patternIndex - badCharShiftTable[text[patternShift + patternIndex]]);
    }

    /*
        If we've checked the entire text and the pattern wasn't found, we set the found_at index to -1 and return from the function
        indicating an unsuccessful search
    */
    searchArgs->found_at = -1;
    
    return 0;
}

/*
    This function generates the bad character shift table which is used in the Boyer Moore string search algorithm.
    The table helps to determine how much to shift the pattern upon mismatch

    Inputs:
        pattern - The pattern string to search for
        patternLength - The length of the pattern
        badCharShiftTable - An array of integers to store the shift values

    Outputs:
        None. It modifies the badCharShiftTable array in-place.

    Usage Example:
        char pattern[]    = "EXAMPLE";
        int patternLength = strlen(pattern);
        int badCharShiftTable[UNIQUE_CHARS];
        generateBadCharacterShiftTable(pattern, patternLength, badCharShiftTable);
*/
void generateBadCharacterShiftTable(char *pattern, int patternLength, int *badCharShiftTable)
{
    /*
        Initialize all entries in the bad character shift table to -1. This is done because in case a character does
        not appear in the pattern, the table should return -1 when accessed with that character
    */
    for (int characterIndex = 0; characterIndex < UNIQUE_CHARS; characterIndex++)
    {
        badCharShiftTable[characterIndex] = -1;
    }

    /*
        For each character in the pattern, set the corresponding entry in the bad character shift table to the latest
        (rightmost) position of that character in the pattern. We use the ASCII value of the character as the index in
        the table. The result is a table that, for each possible character, tells us the rightmost position of that
        character in the pattern
    */
    for (int patternIndex = 0; patternIndex < patternLength; patternIndex++)
    {
        badCharShiftTable[(unsigned char)pattern[patternIndex]] = patternIndex;
    }
}

/*
    This function coordinates the search operation. It reads data from a specified file, divides it into chunks,
    applies the search function to each chunk in a separate thread, and reports the results

    Inputs:
        file_name - The name of the file to be searched
        substring - The substring to be searched for in the file

    Outputs:
        None. Prints to the console whether the substring was found and how long the search took

    Usage Example:
        search_in_file("large_text_file.txt", "specific pattern");
*/
void search_in_file(char* file_name, char* substring)
{
    // Opening the file to gain a HANDLE for file manipulation (specific to Windows and is used to reference various OS objects)
    HANDLE file = CreateFile(file_name, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE)
    {
        DWORD error = GetLastError();
        switch (error)
        {
            case ERROR_FILE_NOT_FOUND:
                printf("File not found: %s\n", file_name);
                break;
            case ERROR_ACCESS_DENIED:
                printf("Access denied: %s\n", file_name);
                break;
            default:
                printf("An unexpected error occurred opening %s: %lu\n", file_name, error);
                break;
        }
        return;
    }

    // Mapping the file into memory to provide faster access, as memory access is faster than disk access
    HANDLE fileMapping = CreateFileMapping(file, NULL, PAGE_READONLY, 0, 0, NULL);
    if (fileMapping == NULL)
    {
        DWORD error = GetLastError();
        printf("Could not create file mapping. Error: %lu\n", error);
        CloseHandle(file);
        return;
    }

    // Mapping the file contents to a character array so it can be processed like a normal string
    char *fileContent = (char *)MapViewOfFile(fileMapping, FILE_MAP_READ, 0, 0, 0);
    if (fileContent == NULL)
    {
        DWORD error = GetLastError();
        printf("Could not map view of file. Error: %lu\n", error);
        CloseHandle(fileMapping);
        CloseHandle(file);
        return;
    }

    // We need the lengths of both the file content and the substring to perform the search
    int textLength    = strlen(fileContent);
    int patternLength = strlen(substring);

    // Handle potential issues that would make the program behave incorrectly
    if (textLength == 0)
    {
        printf("File is empty: %s\n", file_name);
        return;
    }
    else if (patternLength == 0)
    {
        printf("Pattern is empty, nothing to search for.\n");
        return;
    }
    else if (patternLength > textLength)
    {
        printf("Pattern length is greater than the file content length, pattern will not be found.\n");
        return;
    }

    // Generate the bad character shift table once before the search. This is an optimization step in the Boyer-Moore algorithm
    int badCharShiftTable[UNIQUE_CHARS];
    generateBadCharacterShiftTable(substring, patternLength, badCharShiftTable);

    /*
        Dividing the file content into chunks to be processed by different threads.
            - The chunk size is simply the total length divided by the maximum number of threads
            - extra is the length of the remainder which will be added to the last chunk
    */
    int chunkSize  = textLength / MAX_THREADS;
    int extra      = textLength % MAX_THREADS;
    int numThreads = MAX_THREADS;

    // If a chunk is smaller than the pattern length, it would be impossible to find the pattern within it. In such a case, we resort to single-threaded processing
    if (chunkSize < patternLength) 
    {
        chunkSize  = textLength;
        numThreads = 1;
    }

    // Prepare the arguments that will be passed to each search thread
    SearchArgs searchArgs[MAX_THREADS];
    HANDLE threads[MAX_THREADS];

    clock_t start_time = clock();

    /*
        This for loop is used to create a series of threads to perform concurrent searching within different chunks of text from a file. Here's a brief summary of
        what is happening in the loop:
            1. We determine the starting point of the text for each thread to search by offsetting the file content pointer by the chunk size times the thread index
            2. The length of the text to be searched by each thread is decided
                - For the majority of the threads, it is just the chunk size
                - For the last thread, it is assigned the remaining characters which include the chunk size and any extra characters resulting from the division
            3. The pattern to search for is identical for all threads, thus it's assigned to each thread's arguments
            4. The precomputed bad character shift table is copied into the arguments for each search thread
                - This is crucial as it ensures every thread has an independent copy of the table for use in the Boyer-Moore algorithm
            5. found_at is initialized to -1 for each thread (if it ends up being >= 0, it found the substring)
            6. A new thread is created and the search function is assigned to it
                - The relevant argument from the searchArgs array is passed to each thread
    */
    for (int threadIndex = 0; threadIndex < numThreads; threadIndex++) 
    {
        searchArgs[threadIndex].text          = fileContent + (threadIndex * chunkSize);
        searchArgs[threadIndex].textLength    = (threadIndex == numThreads - 1) ? chunkSize + extra : chunkSize;
        searchArgs[threadIndex].pattern       = substring;
        searchArgs[threadIndex].patternLength = patternLength;
        searchArgs[threadIndex].found_at      = -1;
        memcpy(searchArgs[threadIndex].badCharShiftTable, badCharShiftTable, sizeof(badCharShiftTable));
        threads[threadIndex] = (HANDLE)_beginthreadex(NULL, 0, &search, &searchArgs[threadIndex], 0, NULL);
    
        // Check if the thread creation was successful
        if(threads[threadIndex] == 0)
        {
            printf("Thread creation failed.\n");
            
            // Unmap the file view and close handles before exiting
            for (int failedThreadIndex = 0; failedThreadIndex < threadIndex; failedThreadIndex++)
            {
                CloseHandle(threads[failedThreadIndex]);
            }
            UnmapViewOfFile(fileContent);
            CloseHandle(fileMapping);
            CloseHandle(file);
            return;
        }
    }

    // Wait for all threads to finish and check results
    for (int threadIndex = 0; threadIndex < numThreads; threadIndex++) 
    {
        // Wait until the current thread finishes its execution. This prevents the main function from terminating before the threads complete
        WaitForSingleObject(threads[threadIndex], INFINITE);
        CloseHandle(threads[threadIndex]);

        // Check if a thread found the pattern in its designated chunk of the file
        int found_at = searchArgs[threadIndex].found_at;
        if (found_at >= 0) 
        {
            clock_t end_time = clock();
            printf("Found '%s'\nTime taken: %.6f seconds\n", substring, ((double)end_time - start_time) / CLOCKS_PER_SEC);
            return;
        }
    }

    clock_t end_time = clock();
    printf("'%s' not found in the file. Searched in %.6f seconds.\n", substring, ((double)end_time - start_time) / CLOCKS_PER_SEC);

    // Unmap the file view and close handles after the search
    UnmapViewOfFile(fileContent);
    CloseHandle(fileMapping);
    CloseHandle(file);
}

int main()
{
    search_in_file("textfile.txt", "ThisIsTheEnd");
    
    return 0;
}
