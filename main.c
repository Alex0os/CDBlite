#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define COLUMN_USERNAME_SIZE 32
#define COLUMN_EMAIL_SIZE 255

#define size_of_member(Struct, Member) sizeof(((Struct*)0)->Member)

#define PAGE_SIZE 4096
#define TABLE_MAX_PAGES 100


typedef struct {
	uint32_t id;
	char username[COLUMN_USERNAME_SIZE];
	char email[COLUMN_EMAIL_SIZE];
} Row;

typedef struct {
	char* buffer;
	size_t buffer_length;
	size_t input_length;
} InputBuffer;

typedef enum {
	META_COMMAND_SUCCESS,
	META_COMMAND_UNRECOGNIZED_COMMAND
} MetaCommandResult;

typedef enum {
	PREPARE_SUCCESS,
	PREPARE_UNRECOGNIZED_STATEMENT,
	PREPARE_SYNTAX_ERROR
} PrepareResult;

typedef enum {
	STATEMENT_INSERT,
	STATEMENT_SELECT
} StatementType;

typedef struct {
	StatementType type;
	Row row_to_insert;
} Statement;

typedef struct {
	uint32_t num_rows;
	void* pages[TABLE_MAX_PAGES];
} Table;


const uint32_t ID_SIZE = size_of_member(Row, id);
const uint32_t USERNAME_SIZE = size_of_member(Row, username);
const uint32_t EMAIL_SIZE = size_of_member(Row, email);
const uint32_t ID_OFFSET = 0;
const uint32_t USERNAME_OFFSET = ID_OFFSET + ID_SIZE;
const uint32_t EMAIL_OFFSET = USERNAME_OFFSET + USERNAME_SIZE;
const uint32_t ROW_SIZE = ID_SIZE + USERNAME_SIZE + EMAIL_SIZE;

const uint32_t ROWS_PER_PAGE = PAGE_SIZE / ROW_SIZE;
const uint32_t TABLE_MAX_ROW = ROWS_PER_PAGE * TABLE_MAX_PAGES;

// In this point we need to define the compact verision of a row to store them into pages inside memory

void serialize_row(Row* source, void* destination){
	memcpy(source + ID_OFFSET, &(source->id), ID_SIZE);
	memcpy(source + USERNAME_OFFSET, &(source->username), USERNAME_SIZE);
	memcpy(source + EMAIL_OFFSET, &(source->email), EMAIL_SIZE);
}

void deserialize_row(void* destination ,Row* source){
	memcpy(&(source->id), source + ID_OFFSET, ID_SIZE);
	memcpy(&(source->username), source + USERNAME_OFFSET, USERNAME_SIZE);
	memcpy(&(source->email), source + EMAIL_OFFSET, EMAIL_SIZE);
}

InputBuffer* new_input_buffer() {
	InputBuffer* input_buffer = (InputBuffer*)malloc(sizeof(InputBuffer));
	input_buffer->buffer = NULL;
	input_buffer->buffer_length = 0;
	input_buffer->input_length = 0;

	return input_buffer;
}


void close_input_buffer(InputBuffer* input_buffer) {
    free(input_buffer->buffer);
    free(input_buffer);
}


MetaCommandResult do_meta_command(InputBuffer* input_buffer){
	if (strcmp(input_buffer->buffer, ".exit") != 0) {
		return META_COMMAND_UNRECOGNIZED_COMMAND;
	}
	close_input_buffer(input_buffer);
	exit(EXIT_SUCCESS);
}


PrepareResult prepare_statement(InputBuffer* input_buffer, Statement* statement){
	if (strncmp(input_buffer->buffer, "insert", 6) == 0) {
		statement->type = STATEMENT_INSERT;

		// In this section we'll parse the arguments obtained by the prompt
		int arguments_assigned = sscanf(input_buffer->buffer, "insert %d %s %s",
				&(statement->row_to_insert.id),
				&(statement->row_to_insert.username),
				&(statement->row_to_insert.email));
		if (arguments_assigned < 3) {
			return PREPARE_SYNTAX_ERROR;
		}

		return PREPARE_SUCCESS;
	}
	if (strcmp(input_buffer->buffer, "select") == 0) {
		statement->type = STATEMENT_SELECT;
		return PREPARE_SUCCESS;
	}

	return PREPARE_UNRECOGNIZED_STATEMENT;
}


void execute_statement(Statement* statement){
	switch (statement->type) {
		case (STATEMENT_INSERT):
			printf("This is where we would do an insert.\n");
			break;
		case (STATEMENT_SELECT):
			printf("This is where we would do an select.\n");
			break;
	}
}

void read_input(InputBuffer* input_buffer){
	ssize_t bytes_read = getline(&(input_buffer->buffer), &(input_buffer->buffer_length), stdin);

	if (bytes_read <= 0) {
		printf("Error reading input\n");
		exit(EXIT_FAILURE);;
	}

	input_buffer->input_length = bytes_read - 1;
	input_buffer->buffer[bytes_read - 1] = '\0';
}

// Creation of the REPL 
int main(int argc, char *argv[]){
	InputBuffer* input_buffer = new_input_buffer();

	while (1) {
		printf("db > ");
		read_input(input_buffer);

		if (input_buffer->buffer[0] == '.') {
			switch (do_meta_command(input_buffer)) {
				case (META_COMMAND_SUCCESS):
					continue;
				case (META_COMMAND_UNRECOGNIZED_COMMAND):
					printf("Unrecognized command\n");
					continue;
			}
		}

		Statement statement;
		switch (prepare_statement(input_buffer, &statement)) {
			case (PREPARE_SUCCESS):
				break;
			case (PREPARE_UNRECOGNIZED_STATEMENT):
				printf("Unrecognized keyword at the start %s\n", input_buffer->buffer);
				continue;
		}
		execute_statement(&statement);
		printf("Executed\n");
	}
	return 0;
}
