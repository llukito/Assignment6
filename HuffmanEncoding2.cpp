/**********************************************************
 * File: HuffmanEncoding.cpp
 *
 * Implementation of the functions from HuffmanEncoding.h.
 * Most (if not all) of the code that you write for this
 * assignment will go into this file.
 */

#include "HuffmanEncoding.h"
#include "pqueue.h"
#include "simpio.h"
#include <random>

/*
* we will use XOR encryption. We first convert string
* password into number and use hash function so it 
* gives us same value with same password all the time.
* we encrypt bit by bit with nextBit function
*/
class PasswordStream {
public:
	PasswordStream(const string& password) {
		seed = hash<string>{}(password);
		engine.seed(seed);
	}

	bool nextBit(bool originalBit) {
		return originalBit ^ (engine() & 1); // XOR with pseudo-random bit
	}

private:
	size_t seed;
	default_random_engine engine;
};

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
 */
void decodeFile(ibstream& infile, Node* encodingTree, ostream& file) {
	Node* curr = encodingTree;
	while (true) {
		if (!curr->zero && !curr->one) {
			if (curr->character == PSEUDO_EOF) break;
			file.put(curr->character);
			curr = encodingTree;
			continue;
		}
		bool bit = infile.readBit();
		curr = bit ? curr->one : curr->zero;
	}
}

/*
* Instead of freely writing details of map at the start
* of our file, we use XOR encryption, so now map is encrypted
* and can only be decrypted with password ( unless you hack it :) ) 
*/
void writeEncryptedFileHeader(obstream& outfile, Map<ext_char, int>& frequencies, const string& password) {
	if (!frequencies.containsKey(PSEUDO_EOF)) {
		error("No PSEUDO_EOF defined.");
	}

	PasswordStream stream(password);
	int size = frequencies.size() - 1;

	// we write size
	for (int i = 31; i >= 0; i--) {
		bool bit = (size >> i) & 1;
		outfile.writeBit(stream.nextBit(bit));
	}

	for(ext_char ch : frequencies) {
		if (ch == PSEUDO_EOF) continue;

		// we write character
		for (int i = 7; i >= 0; i--) {
			bool bit = (ch >> i) & 1;
			outfile.writeBit(stream.nextBit(bit));
		}

		// then we write frequency
		int freq = frequencies[ch];
		for (int i = 31; i >= 0; i--) {
			bool bit = (freq >> i) & 1;
			outfile.writeBit(stream.nextBit(bit));
		}
	}
}

/*
* This will be used to ready encrypted info, decyrpt it and
* make map. You need correct password to get info correctly,
* otherwise it won't work
*/
Map<ext_char, int> readEncryptedFileHeader(ibstream& infile, const string& password) {
	Map<ext_char, int> result;
	PasswordStream stream(password);

	int numValues = 0;
	for (int i = 0; i < 32; i++) {
		bool bit = infile.readBit();
		bit = stream.nextBit(bit);
		numValues = (numValues << 1) | bit;
	}

	for (int i = 0; i < numValues; i++) {
		ext_char ch = 0;
		for (int j = 0; j < 8; j++) {
			bool bit = infile.readBit();
			bit = stream.nextBit(bit);
			ch = (ch << 1) | bit;
		}

		int freq = 0;
		for (int j = 0; j < 32; j++) {
			bool bit = infile.readBit();
			bit = stream.nextBit(bit);
			freq = (freq << 1) | bit;
		}

		result[ch] = freq;
	}

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
	string password = getLine("Enter password: ");
	writeEncryptedFileHeader(outfile, freqMap, password); 
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
	string password = getLine("password:");
	Map<ext_char, int> freqMap = readEncryptedFileHeader(infile, password);
	Node* root = buildEncodingTree(freqMap);
	decodeFile(infile, root, outfile);                    
	freeTree(root);
}

