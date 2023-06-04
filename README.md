# C Multithreaded Fast Substring Finder
This repository contains an incredibly fast C implementation of the [Boyer-Moore substring search algorithm](https://en.wikipedia.org/wiki/Boyer%E2%80%93Moore_string-search_algorithm). Designed to showcase the rapid search capacity of the Boyer-Moore algorithm, this project is capable of locating a substring at the end of a 112,386,212-character file in an impressive 0.003 - 0.004 seconds. While it does not serve a practical purpose, it does showcase the speed of the Boyer-Moore algorithm in substring searching.

## Getting Started
⚠️ **Important: Windows Only**

Please note that this application is designed to run on Windows systems exclusively. I currently do not guarantee compatibility with other operating systems. If you're using a non-Windows OS, the program might not perform as anticipated. I do plan to extend its compatibility to Linux systems in the future.

To compile the program:
```
gcc -O3 -o substring_search substring_search.c
```

To run the program after it has been compiled:
```
./substring_search
```

## Functionality

The core component of this program is the Boyer-Moore algorithm, known for its efficiency in pattern searching. The algorithm is employed here to locate a specific substring within a text file. Moreover, the program also measures the time it takes to execute the search, providing a clear display of its efficiency.

## Performance

This implementation demonstrates impressive speed. It can search through over 100,000,000 characters for a substring located at the end in 0.003 seconds.

# In-depth Description

The provided C program is designed to efficiently search for a specific substring within a larger text string contained in a file. It employs the Boyer-Moore string search algorithm, a technique known for skipping non-matching sections of text to optimize search performance. The program reads data from a specified text file in chunks, and each chunk is then processed in a separate thread. This multi-threading approach enhances the speed of the program by allowing concurrent execution of the search function across different chunks of the text. If a match is found, the program reports the time taken to complete the search.

The implementation comprises several functions, including one to generate a bad character shift table (a component of the Boyer-Moore algorithm), a core search function to be run as a thread, and a coordinating function to manage the overall search operation. Notably, the search function carries out the heart of the Boyer-Moore algorithm, inspecting characters from the end of the pattern and adjusting its position in the text based on the bad character shift table.

The text file is accessed through Windows-specific functionality for Memory Mapped Files, which provides quicker access than standard disk reading. In terms of performance, the system is designed to manage a large volume of data efficiently, with testing indicating that it can search over 112 million characters in a matter of milliseconds.
