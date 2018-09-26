#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

enum card_colour { 
	CARD_PURPLE, 
	CARD_BROWN, 
	CARD_YELLOW, 
	CARD_RED
};

struct card {
	enum card_colour card_colour;
	int point;
	int cost_PURPLE;
	int cost_BROWN;
	int cost_YELLOW;
	int cost_RED;
};

struct player {
	int pid;
	int fd_to_child;
	int fd_from_child;
};

#define MAX_CARD_DECK_COUNT 1000

struct card card_deck[MAX_CARD_DECK_COUNT];
int card_deck_next_idx = 0;

#define MAX_PLAYERS 100

struct player players[MAX_PLAYERS];
int player_next_idx = 0;

void
usage()
{
	fprintf(stderr, "./austerity <token count> <end point> <deck file name> <player 1> <player 2> <player ....>\n");
	exit(1);
}

enum card_colour
card_char_to_colour(char card)
{
	//printf("card_char_to_colour: %c\n", card);
	switch (card) {
	case 'P': return CARD_PURPLE; break;
	case 'B': return CARD_BROWN; break;
	case 'Y': return CARD_YELLOW; break;
	case 'R': return CARD_RED; break;
	default: errx(1, "Invalid card colour %c", card);
	}

	// should never gets here
	return CARD_PURPLE;
}

char
card_colour_to_char(enum card_colour colour)
{
	switch (colour) {
	case CARD_PURPLE: return 'P'; break;
	case CARD_BROWN: return 'B'; break;
	case CARD_YELLOW: return 'Y'; break;
	case CARD_RED: return 'R'; break;
	default: errx(1, "Invalid card colour %d", colour);
	}

	return 'P';
}

void
add_card_to_deck(char *str) {
	char tmpstr[100], tmpstr2[100];
	int i = 0;
	struct card *card = &card_deck[card_deck_next_idx++];
	char *token, *token2, *saveptr, *saveptr2;

	strncpy(tmpstr, str, 100);
	//printf("tmpstr=%s\n", tmpstr);
	token = strtok_r(tmpstr, ":", &saveptr);

	while (token != NULL) {
		//printf("Token number %d: %s\n", i, token);
		if (i == 0) {
			card->card_colour = card_char_to_colour(token[0]);
		} else if (i == 1) {
			card->point = atoi(token);
		} else if (i == 2) {
			strncpy(tmpstr2, token, 100);
			token2 = strtok_r(tmpstr2, ",", &saveptr2);
			int j = 0;
			while (token2 != NULL) {
				switch (j) {
				case 0: card->cost_PURPLE = atoi(token2); break;
				case 1: card->cost_BROWN = atoi(token2); break;
				case 2: card->cost_YELLOW = atoi(token2); break;
				case 3: card->cost_RED = atoi(token2); break;
				default: errx(1, "Invalid cost (should never get here)"); break;
				}

				token2 = strtok_r(NULL, ",", &saveptr2);
				j++;
			}

			break;
		}

		token = strtok_r(NULL, ":", &saveptr);
		i++;
	}
}

void
print_deck()
{
	int i;
	for (i = 0; i < card_deck_next_idx; i++) {
		printf("%d - %c:%d:%d,%d,%d,%d\n", 
			i + 1,
			card_colour_to_char(card_deck[i].card_colour),
			card_deck[i].point,
			card_deck[i].cost_PURPLE,
			card_deck[i].cost_BROWN,
			card_deck[i].cost_YELLOW,
			card_deck[i].cost_RED);
	}
}

void
read_deck_file(char *deck_file_name)
{
	char str[100];
	int lineno = 1;

	FILE *deck_file = fopen(deck_file_name, "r");

	if (deck_file == NULL) {
		err(1, "Error reading deck file %s", deck_file_name);
	}

	while (fgets(str, 100, deck_file)) {
		str[strlen(str)] = '\0';
		//printf("Read line: %s\n", str);
		lineno++;

		add_card_to_deck(str);
	}

	fclose(deck_file);
}

void
create_player(char *player_file_name, int token_count)
{
	int fd_to_child[2];
	int fd_from_child[2];
	struct player *player;
	int pid = 0;
	int i;
	char buf_out[100];
	char buf_in[100];
	int status;

	if (pipe(fd_to_child)) {
		err(1, "Error creating fd_to_child for %s", player_file_name);
	}

	if (pipe(fd_from_child)) {
		err(1, "Error creating fd_from_child for %s", player_file_name);
	}

	switch (pid = fork()) {
        case -1:
		err(1, "fork");
        case 0:
		// child
		close(fd_to_child[1]);
		dup2(fd_to_child[0], STDIN_FILENO);
		close(fd_to_child[0]);

		close(fd_from_child[0]);
		dup2(fd_from_child[1], STDOUT_FILENO);
		close(fd_from_child[1]);

		execl(player_file_name, NULL);
		break;
        default:
		// parent
		close(fd_to_child[0]);
		close(fd_from_child[1]);

		player = &players[player_next_idx++];
		player->pid = pid;
		player->fd_to_child = fd_to_child[1];
		player->fd_from_child = fd_from_child[0];

		break;
	}

	// only parent gets here
	//sprintf(buf_out, "HELLO CHILD %d, i am your parent %d\n", player->pid, getppid());
	sprintf(buf_out,"tokens%d\n",token_count);

	for (int j = 0; j < 8 ; ++j) {
        sprintf(buf_out, "newcard%c:%d:%d,%d,%d,%d\n",
        		card_colour_to_char(card_deck[j].card_colour),
				card_deck[j].point,
				card_deck[j].cost_PURPLE,
				card_deck[j].cost_BROWN,
				card_deck[j].cost_YELLOW,
				card_deck[j].cost_RED);
    }
	//printf("1\n");
	write(player->fd_to_child, buf_out, 100);
	//printf("2\n");
	read(player->fd_from_child, buf_in, 100);
	//printf("3\n");
	printf("YAY, CHILD %d RESPONDS WITH: %s\n", player->pid, buf_in);

	pid = wait(&status);
	printf("Process %d terminated with status %d\n", pid, status);
}

int 
main(int argc, char **argv)
{
	int i;
	int token_count = -1;
	int end_point;
	char *deck_file_name;
	char **players;
	int player_count;

	if (argc < 6) {
		fprintf(stderr, "Number of arguments not enough\n");
		usage();
	}

	for (i = 0; i < argc; i++) {
		if (i == 0) {
			// this is the name of the file
		} else if (i == 1) {
			token_count = atoi(argv[i]);
			if (token_count == 0) {
				fprintf(stderr, "Invalid token count: %s\n", argv[i]);
				usage();
			}
		} else if (i == 2) {
			end_point = atoi(argv[i]);
			if (end_point == 0) {
				fprintf(stderr, "Invalid end point: %s\n", argv[i]);
				usage();
			}
		} else if (i == 3) {
			deck_file_name = argv[i];
		} else {
			players = &argv[i];
			player_count = argc - i;

			if (player_count < 2) {
				fprintf(stderr, "Player count should be greater than 2\n");
				usage();
			}
			break;
		}
	} 

	read_deck_file(deck_file_name);
	print_deck();

	for (i = 0; i < player_count; i++) {
		create_player(players[i],token_count);
	}

	return 0;
}

