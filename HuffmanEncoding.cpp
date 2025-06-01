/**********************************************************
 * File: HuffmanEncoding.cpp
 *
 * Implementation of the functions from HuffmanEncoding.h.
 * Most (if not all) of the code that you write for this
 * assignment will go into this file.
 */

#include "HuffmanEncoding.h"
#include "pqueue.h"

/* Function: getFrequencyTable
 * Usage: Map<ext_char, int> freq = getFrequencyTable(file);
 * --------------------------------------------------------
 * Given an input stream containing text, calculates the
 * frequencies of each character within that text and stores
 * the result as a Map from ext_chars to the number of times
 * that the character appears.
 *
 * This function will also set the frequency of the PSEUDO_EOF
 * character to be 1, which ensures that any future encoding
 * tree built from these frequencies will have an encoding for
 * the PSEUDO_EOF character.
 */
Map<ext_char, int> getFrequencyTable(istream& file) {
	Map<ext_char, int> freqMap;
	ext_char ch = EOF; // initialize it
	while (true) {
		ch = file.get();
		if (ch == EOF)break;
		freqMap[ch]++;
	}
	freqMap[PSEUDO_EOF] = 1;
 	return freqMap;	
}

/*
* Our helper function for buildEncodingTree. We fill
* priority queue with Nodes.
*/
void fillQueue(PriorityQueue<Node*>& queue, Map<ext_char, int>& frequencies) {
	for (ext_char ch : frequencies) {
		int freq = frequencies[ch];
		Node* node = new Node();
		node->character = ch;
		node->zero = node->one = nullptr;
		node->weight = freq;
		queue.enqueue(node, freq);
	}
}

/* Function: buildEncodingTree
 * Usage: Node* tree = buildEncodingTree(frequency);
 * --------------------------------------------------------
 * Given a map from extended characters to frequencies,
 * constructs a Huffman encoding tree from those frequencies
 * and returns a pointer to the root.
 *
 * This function can assume that there is always at least one
 * entry in the map, since the PSEUDO_EOF character will always
 * be present.
 */
Node* buildEncodingTree(Map<ext_char, int>& frequencies) {
	PriorityQueue<Node*> queue;
	fillQueue(queue, frequencies);
	while (queue.size() > 1) {
		Node* node1 = queue.dequeue();
		Node* node2 = queue.dequeue();
		Node* parentNode = new Node();
		parentNode->character = NOT_A_CHAR;
		parentNode->zero = node1;
		parentNode->one = node2;
		parentNode->weight = node1->weight + node2->weight;
		queue.enqueue(parentNode, parentNode->weight);
	}
	return queue.dequeue();
}

/*
* Our helper function for freeTree. We
* tree our tree recrusively
*/
void freeTreeRec(Node* root) {
	if (!root)return;
	freeTreeRec(root->zero);
	freeTreeRec(root->one);
	delete root;
}

/* Function: freeTree
 * Usage: freeTree(encodingTree);
 * --------------------------------------------------------
 * Deallocates all memory allocated for a given encoding
 * tree.
 */
void freeTree(Node* root) {
	freeTreeRec(root);
}

void fillMap(Map<ext_char, string>& encodeMap, Node* encodingTree, string path) {
	if (!encodingTree) {
		return;
	}
	if (!encodingTree->zero && !encodingTree->one) {
		encodeMap[encodingTree->character] = path;
		return;
	}
	fillMap(encodeMap, encodingTree->zero, path + "0");
	fillMap(encodeMap, encodingTree->one, path + "1");
}

/* Function: encodeFile
 * Usage: encodeFile(source, encodingTree, output);
 * --------------------------------------------------------
 * Encodes the given file using the encoding specified by the
 * given encoding tree, then writes the result one bit at a
 * time to the specified output file.
 *
 * This function can assume the following:
 *
 *   - The encoding tree was constructed from the given file,
 *     so every character appears somewhere in the encoding
 *     tree.
 *
 *   - The output file already has the encoding table written
 *     to it, and the file cursor is at the end of the file.
 *     This means that you should just start writing the bits
 *     without seeking the file anywhere.
 */ 
void encodeFile(istream& infile, Node* encodingTree, obstream& outfile) {
	Map<ext_char, string> encodeMap;
	fillMap(encodeMap, encodingTree, "");
	ext_char ch = EOF; // initialize it
	while (true) {
		ch = infile.get();
		if (ch == EOF)break;
		string code = encodeMap[ch];
		for (char c : code) {
			outfile.writeBit(c == '1');
		}
	}
	string endCode = encodeMap[PSEUDO_EOF];
	for (char c : endCode) {
		outfile.writeBit(c == '1');
	}
}

/* Function: decodeFile
 * Usage: decodeFile(encodedFile, encodingTree, resultFile);
 * --------------------------------------------------------
 * Decodes a file that has previously been encoded using the
 * encodeFile function.  You can assume the following:
 *
 *   - The encoding table has already been read from the input
 *     file, and the encoding tree parameter was constructed from
 *     this encoding table.
 *
 *   - The output file is open and ready for writing.
 * 
 * I think our program should assume that empty file is not
 * passed to decode but I still added check for it
 */
void decodeFile(ibstream& infile, Node* encodingTree, ostream& file) {
	// we check if there is only one (PSEUDO_EOF) node 
	if (encodingTree->character == PSEUDO_EOF) {
		error("File is empty");
		return;
	}
	Node* curr = encodingTree;
	while (true) {
		bool bit = infile.readBit();
		if (bit) {
			curr = curr->one;
		}
		else {
			curr = curr->zero;
		}
		if (!curr->zero && !curr->one) {
			ext_char ch = curr->character;
			if (ch == PSEUDO_EOF)break;
			file.put(ch);
			curr = encodingTree;
		}
	}
}

/* Function: writeFileHeader
 * Usage: writeFileHeader(output, frequencies);
 * --------------------------------------------------------
 * Writes a table to the front of the specified output file
 * that contains information about the frequencies of all of
 * the letters in the input text.  This information can then
 * be used to decompress input files once they've been
 * compressed.
 *
 * This function is provided for you.  You are free to modify
 * it if you see fit, but if you do you must also update the
 * readFileHeader function defined below this one so that it
 * can properly read the data back.
 */
void writeFileHeader(obstream& outfile, Map<ext_char, int>& frequencies) {
	/* The format we will use is the following:
	 *
	 * First number: Total number of characters whose frequency is being
	 *               encoded.
	 * An appropriate number of pairs of the form [char][frequency][space],
	 * encoding the number of occurrences.
	 *
	 * No information about PSEUDO_EOF is written, since the frequency is
	 * always 1.
	 */
	 
	/* Verify that we have PSEUDO_EOF somewhere in this mapping. */
	if (!frequencies.containsKey(PSEUDO_EOF)) {
		error("No PSEUDO_EOF defined.");
	}
	
	/* Write how many encodings we're going to have.  Note the space after
	 * this number to ensure that we can read it back correctly.
	 */
	outfile << frequencies.size() - 1 << ' ';
	
	/* Now, write the letter/frequency pairs. */
	foreach (ext_char ch in frequencies) {
		/* Skip PSEUDO_EOF if we see it. */
		if (ch == PSEUDO_EOF) continue;
		
		/* Write out the letter and its frequency. */
		outfile << char(ch) << frequencies[ch] << ' ';
	}
}

/* Function: readFileHeader
 * Usage: Map<ext_char, int> freq = writeFileHeader(input);
 * --------------------------------------------------------
 * Reads a table to the front of the specified input file
 * that contains information about the frequencies of all of
 * the letters in the input text.  This information can then
 * be used to reconstruct the encoding tree for that file.
 *
 * This function is provided for you.  You are free to modify
 * it if you see fit, but if you do you must also update the
 * writeFileHeader function defined before this one so that it
 * can properly write the data.
 */
Map<ext_char, int> readFileHeader(ibstream& infile) {
	/* This function inverts the mapping we wrote out in the
	 * writeFileHeader function before.  If you make any
	 * changes to that function, be sure to change this one
	 * too!
	 */
	Map<ext_char, int> result;
	
	/* Read how many values we're going to read in. */
	int numValues;
	infile >> numValues;
	
	/* Skip trailing whitespace. */
	infile.get();
	
	/* Read those values in. */
	for (int i = 0; i < numValues; i++) {
		/* Get the character we're going to read. */
		ext_char ch = infile.get();
		
		/* Get the frequency. */
		int frequency;
		infile >> frequency;
		
		/* Skip the space character. */
		infile.get();
		
		/* Add this to the encoding table. */
		result[ch] = frequency;
	}
	
	/* Add in 1 for PSEUDO_EOF. */
	result[PSEUDO_EOF] = 1;
	return result;
}

/* Function: compress
 * Usage: compress(infile, outfile);
 * --------------------------------------------------------
 * Main entry point for the Huffman compressor.  Compresses
 * the file whose contents are specified by the input
 * ibstream, then writes the result to outfile.  Your final
 * task in this assignment will be to combine all of the
 * previous functions together to implement this function,
 * which should not require much logic of its own and should
 * primarily be glue code.
 */
void compress(ibstream& infile, obstream& outfile) {
	Map<ext_char, int> freqMap = getFrequencyTable(infile);
	Node* root = buildEncodingTree(freqMap);
	writeFileHeader(outfile, freqMap);
	infile.rewind();
	encodeFile(infile, root, outfile);
	freeTree(root);
}

/* Function: decompress
 * Usage: decompress(infile, outfile);
 * --------------------------------------------------------
 * Main entry point for the Huffman decompressor.
 * Decompresses the file whose contents are specified by the
 * input ibstream, then writes the decompressed version of
 * the file to the stream specified by outfile.  Your final
 * task in this assignment will be to combine all of the
 * previous functions together to implement this function,
 * which should not require much logic of its own and should
 * primarily be glue code.
 */
void decompress(ibstream& infile, ostream& outfile) {
	Map<ext_char, int> freqMap = readFileHeader(infile);
	Node* root = buildEncodingTree(freqMap);
	decodeFile(infile, root, outfile);
	freeTree(root);
}

