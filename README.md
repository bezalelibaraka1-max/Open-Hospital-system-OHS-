# Open Hospital system
An open-source hospital system developed mainly in C language

## Features
-Doctor panel
-Pharmacy panel
-Reception panel
-file based storage(later it will include database storage and custom file storage)
-Use C sockets to communicate across panels
-It is still very basic and Raw where by u can compile the individual panels ,not a whole system in one
-The user interface is made using GTK-3 and glade
-The system desires is to support multiple languages not only C language..(working on it)
## Requirements
-C language(primarily)
-Mysql
-Other languages which may communicate with C clearly through api or any other technology

## Installation
1.Clone this repository
2.Compile individual components because the system is not combined as one yet.
3.Example of compilation gcc main.c -o main `pkg-config --cflags --libs gtk+-3.0`
4.Run using ./main
5.Make sure you are in the folder of the files which you are compiling.

## How to contibute
1. Fork this repository.
2. Clone your fork.
3. Create a new branch.
4. Make your changes.
5. Test your changes.
6. Commit your changes.
7. Push your branch.
8. Open a Pull Request.

## Branch naming
Use descriptive branch names
-UI/pharmacy
-improvement/protocol

## Commit messages
Use clear commit messages like "Fixing bug in doctor-panel"

## License
MIT license
