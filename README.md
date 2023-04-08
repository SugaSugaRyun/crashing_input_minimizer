CIMIN
====

cimin (Crashing Input Minimizer) is a C program that, when an input to a program causes a conflict, simplifies that input by removing many of the non-conflicting parts of the input.

Install and Build
-----
To clone the repository you should have Git installed. Just run:

    $ git clone https://github.com/SugaSugaRyun/crashing_input_minimizer.git

To build the library, run `make`. 

    $ make

To delete *.o files and excutable file(cimin), run
`make clean`.

    $ make clean

How to use
----

    $ ./cimin -i [crashing input path] -m "[standard error message]" -o [new output file path] ./[testing program] [argvs of test program] ... ...

You should excute cimin with some command line arguments.

> argv[1] ~ argv[6]  
>> -i [crashing input path] : Your input is provided to the target program as standard input using the file<br>  
> -m  [standard error message] : The string contained in the error message you will receive from the target program. (i.e. heap-buffer-overflow)<br>  
> -o  [new output file path] : The program returns the reduced input to a file.  
>
> argv[7] ~
>> ./[testing program] : Crashing program<br>  
> ... ... : Testing program's command line arguments<br>  
