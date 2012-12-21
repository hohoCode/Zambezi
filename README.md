Query File
------------

A query file must follow the following format:

	<first_line> .=. <Number of queries:integer>
	<line> .=. <query id: integer> <query length: integer> <query: text>

Building an Inverted Index
--------------

The input to the index must be a set of (gzipped or raw) text files, where each line contains one document in the following format:

	<document_id: integer> \t <document: text>.

Please note that you must perform necessary preprocessing (e.g., parsing, stopping, stemming) prior to using the indexer, as the index is only able to read parsed documents and does not perform any sort of stopping or stemming.

To run the indexer:

	out/driver/indexer -index <output-index-root-path> [-positional]
	-mb <maximum-buffer-length-in-number-of-blocks> -input <input-paths>

Note that `-input` must be the last argument, and that `<input-paths>` is a list of files.

You can create a contiguous index as follows:

	out/driver/buildContiguous -input <input-index-root-path> -output <output-index-root-path>

Retrieval
----------------------

To do retrieval:

	out/driver/retrieval -index <index-root-path> -query <query-path> -algorithm <SvS>
	[-output <output-path>]

If `-output` is included, the output is stored at `<output-path>`.