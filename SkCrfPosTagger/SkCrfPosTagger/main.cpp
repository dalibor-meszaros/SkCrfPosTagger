// author: Dalibor Mészáros
// name: Bachelor's thesis - Determining the Parts of Speech in Slovak Language
//       Bakalárska práca - Určovanie slovných druhov v Slovenčine

//#define LANGUAGE_SLOVAK

#include <stdio.h>
#include <stdlib.h>

// implementation of word2vec
#include "vlib.h"

// custom header file containing functions for std::(w)string manipulation, etc.
#include "denralib.h"

// objekt sluziaci na uchovanie slova a jeho crt v podobe tokenu
typedef struct token {
	std::wstring word;					// origin word
	std::wstring word_lowercase;		// lowercase word
	BOOL is_first_upper;				// if first letter is capital
	BOOL is_full_upper;					// if the word is in uppercase
	BOOL is_semi_upper;					// if the word contains other capital letter
	BOOL contains_punct;				// if the word contains symbols
	BOOL contains_digit;				// if the word contains numbers
	size_t word_length;					// word length
	size_t sentence_position;			// position of the word in sentence
	std::wstring prefix[4];				// preffix of the word 1-4
	std::wstring suffix[4];				// suffix of the word 1-4
	int *vector;						// pointer for array containing vector, if we use them
}TOKEN;

// global variables
std::wstring gv_path_in = L"",
gv_path_out = L"";
FILE *gv_file_out = NULL;
BOOL gv_use_vector = FALSE,
gv_use_mapping = FALSE;

// help message
inline void PrintHelp(std::string &arg_program_name) {
#ifdef LANGUAGE_SLOVAK
	printf("Pouzitie: %s [MOZNOSTI...] VSTUP\n", arg_program_name.c_str());
	puts("Rozdeli text a urci slovne druhy pre kazde slovo/znak.\n\n"
		"  -f, --file\tnacita text zo suboru, miesto vstupu\n"
		"  -o, --out\tvypise vystup do suboru, miesto konzoly\n"
		"  -m, --map\tvypise slova a prisluchajuce znacky, miesto len znaciek\n"
		"  -v, --vector\tpouzije sa vectorom trenovany model\n"
		"  -h, --help\tzobrazi tuto pomoc");
#else
	printf("Usage: %s [OPTION...] INPUT\n", arg_program_name.c_str());
	puts("Tokenizes input text and determines slovak parts of speech for each token.\n\n"
		"  -f, --file\treats the text from file rather then standard input\n"
		"  -o, --out\toutputs processed text to file without messages\n"
		"  -m, --map\toutputs word with pos tag, instead of only tag\n"
		"  -v, --vector\tuse model trained with vectors\n"
		"  -h, --help\tdisplay this help and exit");
#endif
}

// vypise jednotlive znacky s ich vyznamom
inline void PrintTagsExplanation() {
#ifdef LANGUAGE_SLOVAK
	puts("\nTag's meaning:\n"
		"[S] Substantivum; Podstatne meno\n"
		"[A] Adjektivum; Pridavne meno\n"
		"[P] Pronominum; Zameno\n"
		"[N] Numerale; Cislovka\n"
		"[S] Verbum; Sloveso\n"
		"[G] Participium; Pricastie\n"
		"[D] Adverbium; Prislovka\n"
		"[E] Prepozicia; Predlozka\n"
		"[O] Konjunkcia; Spojka\n"
		"[T] Partikula; Castica\n"
		"[J] Interjekcia; Citoslovce\n"
		"[R] Reflexivum\n"
		"[Y] Kondicionalna morfema\n"
		"[W] Abreviacie; Znacky\n"
		"[Z] Interpunkcia\n"
		"[Q] Neurcitelny slovny druh\n"
		"[#] Neslovny element\n"
		"[%] Citatovy vyraz\n"
		"[0] Cislica\n");
#else
	puts("\nTag's meaning:\n"
		"[S] Substantives\n"
		"[A] Adjectives\n"
		"[P] Pronouns\n"
		"[N] Numerals\n"
		"[S] Verbs\n"
		"[G] Participles\n"
		"[D] Adverbs\n"
		"[E] Prepositions\n"
		"[O] Conjunctions\n"
		"[T] Particles\n"
		"[J] Interjections\n"
		"[R] Reflexives\n"
		"[Y] Conditional morpheme\n"
		"[W] Abbreviations\n"
		"[Z] Punctuations\n"
		"[Q] Undeterminable parts of speech\n"
		"[#] Non-word elements\n"
		"[%] Citations\n"
		"[0] Numbers\n");
#endif
}

// interprest the input, sets paths and raises flags
void InterpretParameters(int &argc, char ** &argv) {
	int arg_iter;
	FILE * file_in = NULL;
	std::string program_name = argv[0], path_base, buffer_ascii;

	program_name = program_name.substr(program_name.rfind("\\") + 1);
	program_name = program_name.substr(0, program_name.rfind("."));

	if (argc < 2) {
		printf("%s: missing arguments\n\n", program_name.c_str());
		PrintHelp(program_name);
		exit(EXIT_SUCCESS);
	}

	for (arg_iter = 1; arg_iter < argc; ++arg_iter) {
		// saves path and opens input file
		if (strcmp(argv[arg_iter], "-f") == 0 || strcmp(argv[arg_iter], "--file") == 0) {
			++arg_iter;
			buffer_ascii = argv[arg_iter];
			std::wstring buffer_wide(buffer_ascii.begin(), buffer_ascii.end());
			gv_path_in = buffer_wide;
			if ((file_in = _wfopen(gv_path_in.c_str(), FOPEN_MODE_READ_UTF8_W)) == NULL) {
				fprintf(stderr, "Error: Unable to open %s\n\n", argv[arg_iter]);
				exit(EXIT_ERROR_FOPEN);
			}
			fclose(file_in);
		}
		// saves path and opens output file
		else if (strcmp(argv[arg_iter], "-o") == 0 || strcmp(argv[arg_iter], "--out") == 0) {
			++arg_iter;
			buffer_ascii = argv[arg_iter];
			std::wstring buffer_wide(buffer_ascii.begin(), buffer_ascii.end());
			gv_path_out = buffer_wide;
			// opens the directory to the path
			path_base = buffer_ascii.substr(buffer_ascii.rfind("\\") + 1);
			if (path_base != buffer_ascii)
				DirectoryCreateSys(path_base.c_str());
			else
				gv_path_out = L".\\" + gv_path_out;
			if ((gv_file_out = _wfopen(gv_path_out.c_str(), FOPEN_MODE_WRITE_UTF8_W)) == NULL) {
				fprintf(stderr, "Error: Unable to create %s\n\n", argv[arg_iter]);
				exit(EXIT_ERROR_FOPEN);
			}
		}
		// enable word-tag mapping?
		else if (strcmp(argv[arg_iter], "-m") == 0 || strcmp(argv[arg_iter], "--map") == 0) {
			gv_use_mapping = TRUE;
		}
		// use vector model?
		else if (strcmp(argv[arg_iter], "-v") == 0 || strcmp(argv[arg_iter], "--vector") == 0) {
			gv_use_vector = TRUE;
		}
		// print help
		else if (strcmp(argv[arg_iter], "-h") == 0 || strcmp(argv[arg_iter], "--help") == 0) {
			wprintf(L"%S (C) Dalibor Meszaros\n\n", program_name.c_str());
			PrintHelp(program_name);
			PrintTagsExplanation();
			exit(EXIT_SUCCESS);
		}
		else if (argv[arg_iter][0] == '-') {
			fprintf(stderr, "Error: Unknown switch %s\n", argv[arg_iter]);
			exit(EXIT_ERROR_INPUT);
		}
		else
			break;
	}

	if (gv_path_in.empty() && arg_iter + 1 == argc) // if last argument is text
		return;
	else if (!gv_path_in.empty()) // if we have input file argument
		return;
	else {
		fprintf(stderr, "Error: Unknown argument %s\n", argv[arg_iter]);
		exit(EXIT_ERROR_INPUT);
	}
}

void SaveInput(int &argc, char ** &argv) {
	FILE * file_in;
	std::string buffer_ascii;

	if (gv_path_in.empty()) {
		gv_path_in = L"~temp\\temp_input.txt";
		buffer_ascii = argv[argc - 1];
		std::wstring input(buffer_ascii.begin(), buffer_ascii.end());
		if ((file_in = _wfopen(gv_path_in.c_str(), FOPEN_MODE_WRITE_UTF8_W)) == NULL) {
			DirectoryDeleteSys("~temp");
			exit(EXIT_ERROR_FOPEN);
		}
		fwprintf(file_in, L"%s", input.c_str());
		fclose(file_in);
	}
}

void Tokenize(std::wstring &arg_str) {
	std::string path_in_ascii(gv_path_in.begin(), gv_path_in.end()),
		command = std::string() + "java \"edu.stanford.nlp.process.DocumentPreprocessor\" -tokenizerOptions \"asciiQuotes=true\" \"" + path_in_ascii.c_str() + "\" | java \"edu.stanford.nlp.process.PTBTokenizer\" -options \"tokenizeNLs=true,asciiQuotes=true\" >\"~temp\\temp_tokenizer.txt\"";
	system(command.c_str());
	std::wstring file_path = L"~temp\\temp_tokenizer.txt";
	FileLoad(file_path.c_str(), FOPEN_MODE_READ_UTF8_W, arg_str);
	StringReplaceAllAlter(arg_str, L"\n*NL*\n", L"\n\n");
}

// initializes vlib.h, vector file and variables required
void VlibInitialize(int **arg_id, float **arg_vector, float **arg_dist, float **arg_rel, int vector_n_max) {
	read_vectors("vec-300sk.bin");
	if ((*arg_vector = (float*)malloc(vsize*sizeof(float))) == NULL) {
		fprintf(stderr, "Error: Unable to allocate %d for arg_vector\n", vsize*sizeof(float));
		exit(EXIT_ERROR_MALLOC);
	}
	if ((*arg_id = (int*)malloc(vector_n_max*sizeof(int))) == NULL)
		fprintf(stderr, "Error: Unable to allocate %d for arg_id\n", vector_n_max*sizeof(int));
	exit(EXIT_ERROR_MALLOC);
	if ((*arg_dist = (float*)malloc(vector_n_max*sizeof(float))) == NULL)
		fprintf(stderr, "Error: Unable to allocate %d for arg_id\n", vector_n_max*sizeof(float));
	exit(EXIT_ERROR_MALLOC);
	if ((*arg_rel = (float*)malloc(words*sizeof(float))) == NULL)
		fprintf(stderr, "Error: Unable to allocate %d for arg_rel\n", words*sizeof(float));
	exit(EXIT_ERROR_MALLOC);
}

// sets binary variables for token
void StringAbout(TOKEN &arg_token) {
	int i;
	size_t len = arg_token.word.length();
	arg_token.is_full_upper = TRUE;
	arg_token.is_semi_upper = arg_token.contains_punct = arg_token.contains_digit = FALSE;

	// is first upper
	if (iswpunct(arg_token.word.at(FALSE))) {
		arg_token.contains_punct = TRUE;
		arg_token.is_full_upper = arg_token.is_first_upper = FALSE;
	}
	else if (iswdigit(arg_token.word.at(FALSE))) {
		arg_token.contains_digit = TRUE;
		arg_token.is_full_upper = arg_token.is_first_upper = FALSE;
	}
	else if (iswupper(arg_token.word.at(FALSE))) {
		arg_token.is_semi_upper = arg_token.is_first_upper = TRUE;
	}
	else {
		arg_token.is_full_upper = arg_token.is_first_upper = FALSE;
	}

	// is full or semiupper
	for (i = TRUE; i < len; ++i) {
		if (iswpunct(arg_token.word.at(i))) {
			arg_token.contains_punct = TRUE;
			arg_token.is_full_upper = FALSE;
		}
		else if (iswdigit(arg_token.word.at(i))) {
			arg_token.contains_digit = TRUE;
			arg_token.is_full_upper = FALSE;
		}
		else if (iswupper(arg_token.word.at(i))) {
			arg_token.is_semi_upper = TRUE;
		}
		else {
			arg_token.is_full_upper = FALSE;
		}
	}
}

void PreprocessText(std::wstring arg_str, size_t &arg_tokens_count, TOKEN * &arg_tokens) {
	FILE * file;
	size_t i, j,
		start, end = 0, sentence_position = 0,
		cut;
	int k, l;
	std::wstring word_stripped,
		buffer_token = L"", temp_path;
	wchar_t buffer[2048];
	float *vector, *dist, *rel;
	int *vec_id = NULL, word_id, ret_k;

	temp_path = L"~temp\\temp_crf.txt";
	if ((file = _wfopen(temp_path.c_str(), FOPEN_MODE_WRITE_UTF8_W)) == NULL) {
		exit(EXIT_ERROR_FOPEN);
	}

	arg_tokens_count = 0;
	for (i = 0; i < arg_str.size(); ++i) {
		if (arg_str[i] == '\n')
			++arg_tokens_count;
	}

	arg_tokens = new TOKEN[arg_tokens_count];

	// fill array with tokens from text
	for (i = 0; i < arg_tokens_count; ++i) {
		start = end;
		end = arg_str.find(L"\n", start);
		if (end - start == 0) {
			++end;
			sentence_position = -1;
			arg_tokens[i].word = '\n';
		}
		else {
			++end;
			arg_tokens[i].sentence_position = sentence_position;
			arg_tokens[i].word = arg_str.substr(start, end - start - 1);
		}
		++sentence_position;
	}

	// initialize vector if we need it
	if (gv_use_vector)
		VlibInitialize(&vec_id, &vector, &dist, &rel, 20);

	// generate features for tokens
	for (i = 0; i < arg_tokens_count; ++i) {
		if (arg_tokens[i].word == L"\n")
			continue;
		arg_tokens[i].word_lowercase = StringToLower(arg_tokens[i].word);
		StringReplaceAllAlter((arg_tokens[i].word_lowercase), L" ", L"_");
		StringReplaceAllAlter((arg_tokens[i].word_lowercase), L"|", L"*");
		StringReplaceAllAlter((arg_tokens[i].word_lowercase), L"\\", L"\\\\");
		StringReplaceAllAlter((arg_tokens[i].word_lowercase), L":", L"\\:");
		StringAbout(arg_tokens[i]);
		arg_tokens[i].word_length = (arg_tokens[i].word_lowercase).length();
		for (j = 0; j < 4; ++j) {
			cut = j + 1;
			(arg_tokens[i].prefix)[j] = (arg_tokens[i].word_lowercase).substr(0, cut);
			if (arg_tokens[i].word_length < cut)
				(arg_tokens[i].suffix)[j] = (arg_tokens[i].word_lowercase).substr(0, cut);
			else
				(arg_tokens[i].suffix)[j] = (arg_tokens[i].word_lowercase).substr(arg_tokens[i].word_length - cut, cut);
		}
		if (gv_use_vector) {
			word_stripped = arg_tokens[i].word;
			// remove diacritics for vlib.h
			StringRemoveDiacriticsAlter(word_stripped);
			std::string word_ascii(word_stripped.begin(), word_stripped.end());
			word_id = get_word_index(word_ascii.c_str(), vector);
			if (word_id >= 0) {
				ret_k = k_nearest3(vector, 20, vec_id, dist, word_id);
			}
			arg_tokens[i].vector = new int[20];
			for (j = 0; j < 20; ++j) {
				if ((word_id < 0) || (j > ret_k))
					(arg_tokens[i].vector)[j] = -1;
				else
					(arg_tokens[i].vector)[j] = vec_id[j];
			}
		}
	}

	for (i = 0; i < arg_tokens_count; ++i) {
		if (arg_tokens[i].word == L"\n") {
			continue;
		}

		buffer_token += L"X";
		for (k = -2; k <= 2; ++k) {
			if ((i < k * (-1)) || (i + k >= arg_tokens_count))
				continue;
			swprintf(buffer, L"\tw[%d]=%s", k, arg_tokens[i + k].word_lowercase.c_str());
			buffer_token += buffer;
		}
		for (k = -2; k <= 2; ++k) {
			if ((i < k * (-1)) || (i + k >= arg_tokens_count))
				continue;
			swprintf(buffer, L"\tf0[%d]=%d", k, arg_tokens[i + k].is_first_upper);
			buffer_token += buffer;
		}
		for (k = -2; k <= 2; ++k) {
			if ((i < k * (-1)) || (i + k >= arg_tokens_count))
				continue;
			swprintf(buffer, L"\tf1[%d]=%d", k, arg_tokens[i + k].is_full_upper);
			buffer_token += buffer;
		}
		for (k = -2; k <= 2; ++k) {
			if ((i < k * (-1)) || (i + k >= arg_tokens_count))
				continue;
			swprintf(buffer, L"\tf2[%d]=%d", k, arg_tokens[i + k].is_semi_upper);
			buffer_token += buffer;
		}
		for (k = -2; k <= 2; ++k) {
			if ((i < k * (-1)) || (i + k >= arg_tokens_count))
				continue;
			swprintf(buffer, L"\tf3[%d]=%d", k, arg_tokens[i + k].contains_punct);
			buffer_token += buffer;
		}
		for (k = -2; k <= 2; ++k) {
			if ((i < k * (-1)) || (i + k >= arg_tokens_count))
				continue;
			swprintf(buffer, L"\tf4[%d]=%d", k, arg_tokens[i + k].contains_digit);
			buffer_token += buffer;
		}
		for (k = -2; k <= 2; ++k) {
			if ((i < k * (-1)) || (i + k >= arg_tokens_count))
				continue;
			swprintf(buffer, L"\tf5[%d]=%llu", k, arg_tokens[i + k].word_length);
			buffer_token += buffer;
		}
		for (k = -2; k <= 2; ++k) {
			if ((i < k * (-1)) || (i + k >= arg_tokens_count))
				continue;
			swprintf(buffer, L"\tf6[%d]=%llu", k, arg_tokens[i + k].sentence_position);
			buffer_token += buffer;
		}
		for (l = 0; l < 4; ++l) {
			for (k = -2; k <= 2; ++k) {
				if ((i < k * (-1)) || (i + k >= arg_tokens_count))
					continue;
				swprintf(buffer, L"\tf%d[%d]=%s", l + 7, k, (arg_tokens[i + k].prefix)[l].c_str());
				buffer_token += buffer;
			}
		}
		for (l = 0; l < 4; ++l) {
			for (k = -2; k <= 2; ++k) {
				if ((i < k * (-1)) || (i + k >= arg_tokens_count))
					continue;
				swprintf(buffer, L"\tf%d[%d]=%s", l + 11, k, (arg_tokens[i + k].suffix)[l].c_str());
				buffer_token += buffer;
			}
		}
		if (gv_use_vector) {
			for (l = 0; l < 20; ++l) {
				for (k = -2; k <= 2; ++k) {
					if ((i < k * (-1)) || (i + k >= arg_tokens_count))
						continue;
					swprintf(buffer, L"\tv[%d]=%d", k, (arg_tokens[i + k].vector)[l]);
					buffer_token += buffer;
				}
			}
		}

		if ((i <= (arg_tokens_count + 1)) && (arg_tokens[i].sentence_position >= arg_tokens[i + 1].sentence_position))
			buffer_token += L"\t__EOS__\n\n";
		else if (arg_tokens[i].sentence_position == 0)
			buffer_token += L"\t__BOS__\n";
		else
			buffer_token += L"\n";

		fputws(buffer_token.c_str(), file);
		buffer_token = L"";
	}

	fclose(file);
}

void CrfTag(std::wstring &arg_output) {
	std::wstring command = L"crfsuite.exe tag -m \"";
	if (gv_use_vector) {
		command += L"crf-vec-1pct.mdl\" \"~temp\\temp_crf.txt\"";
	}
	else {
		command += L"crf-10pct.mdl\" \"~temp\\temp_crf.txt\"";
	}
	arg_output = Execute(command.c_str());
}

// prints to output or file; formats the output differently based on map flag
void OutputTags(std::wstring &arg_output, size_t arg_tokens_count, TOKEN * &arg_tokens) {
	size_t i, tag_iter = 0;
	if (gv_use_mapping) {
		StringReplaceAllAlter(arg_output, L"\n", L"");
		StringReplaceAllAlter(arg_output, L"\r", L"");
		if (gv_path_out.empty()) {
			for (i = 0; i < arg_tokens_count; ++i) {
				if (arg_tokens[i].word == L"\n") {
					putchar('\n');
				}
				else {
					wprintf(L"%s %c\n", arg_tokens[i].word.c_str(), arg_output.at(tag_iter));
					++tag_iter;
				}
			}
		}
		else {
			for (i = 0; i < arg_tokens_count; ++i) {
				if (arg_tokens[i].word == L"\n") {
					putchar('\n');
				}
				else {
					fwprintf(gv_file_out, L"%s %c\n", arg_tokens[i].word.c_str(), arg_output.at(tag_iter));
					++tag_iter;
				}
			}
			fclose(gv_file_out);
		}
	}
	else {
		if (gv_path_out.empty()) {
			wprintf(L"%s", arg_output.c_str());
		}
		else {
			fwprintf(gv_file_out, L"%s", arg_output.c_str());
			fclose(gv_file_out);
		}
	}
}

int main(int argc, char *argv[]) {
	std::wstring input, output;
	size_t tokens_count = 0;
	TOKEN * tokens;

	SET_LOCALE("slovak");
	InterpretParameters(argc, argv);

	DirectoryCreateSys("~temp");

	SaveInput(argc, argv);
	Tokenize(input);
	PreprocessText(input, tokens_count, tokens);
	CrfTag(output);
	OutputTags(output, tokens_count, tokens);

	DirectoryDeleteSys("~temp");

	return EXIT_SUCCESS;
}