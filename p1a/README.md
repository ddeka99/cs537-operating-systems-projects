# CS 537 Project 1A: Warm-up Project (Linux)

Project description can be found [here](https://pages.cs.wisc.edu/~remzi/Classes/537/Fall2021/Projects/p1a.html)

## Building and Testing

- Run `make` to build the project.
- Results from running the testsuite can be found in `tests-out` directory.

## Intro

- `database.txt` is used as persistent storage memory.
- A linkedlist datastructure is used for the described operations.
- kv-pair implies key-value pair.
- KVnode refers to the node structure used for the linkedlist.

## Function descriptions

- Functions created to implement the various functionalities of the above mentioned kv-dictionary are described below.

### int main(int argc, char **argv)

- Driver function.
- Accepts commands through command line arguments.
- Checks for existing `database.txt`. If not present, creates it. If present, loads it onto the linkedlist.
- Extracts command key and other necessary arguments.
- Run operation according to the extracted command key.

### void LoadDatabase(void)

- Reads `database.txt` line by line.
- Tokenizes each line into kv-pairs.
- Appends each kv-pair to the end of linkedlist.

### void WriteDatabase(void)

- Write each node in the linkedlist into `database.txt` in the form of kv-pairs.

### KVnode *AppendKVNode(KVnode*, int, char*)

- Reserve memory for new node.
- Extract kv-pair from the passed KVnode.
- Appends the kv-pair to the end of the linkedlist.
- Update tail pointer.

### void PutEntry(int)

- Function for handling operation for command key 'p'.
- Extract kv-pair from the passed argument.
- If duplicate key, delete older entry.
- Append kv-pair to the end of the linkedlist.

### void GetEntry(int)

- Function for handling operation for command key 'g'.
- Extract key from the passed argument.
- Search for the key in the linkedlist.
- If found, return value for that key.

### KVnode *DeleteEntry(KVnode*, int)

- Function for handling operation for command key 'd'.
- Extract key from the passed argument.
- Search for the key in the linkedlist.
- If found, delete the kv-pair from the linkedlist.
- Handle case when kv-pair to be deleted is the head of the linkedlist.
- Handle case when kv-pair to be deleted is the last item in the linkedlist.

### void ClearEntries()

- Function for handling operation for command key 'c'.
- Delete all nodes in the linkedlist.

### void PrintKVNodes(KVnode*)

- Function for handling operation for command key 'a'.
- Print all kv-pairs from the nodes in the linkedlist.

## Extra

- Used `strsep()` for tokenizing strings.
- Used `atoi()` for parsing strings as integers. Problematic function since it can't differentiate between 0 and error. Should be updated in the future.
