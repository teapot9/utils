/*
 * main.c - remount
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

#ifndef __linux__
#error This program is compatible with Linux systems only
#endif

#if defined(__GNUC__) || defined(__clang__)
#define noreturn __attribute__((noreturn))
#else
#define noreturn
#endif

#define MAX(X, Y) ((X < Y) ? Y : X)

// Static configurations
#define CONFIG_FILE "/etc/remount.conf"
#define DEFCONFIG_FILE "/usr/local/share/remount/remount.conf.default"
#define MOUNTPOINT_MAX_COUNT 16
#define MOUNTPOINT_MAX_PATH 256

// Remount command
#define REMOUNT_COMMAND_START "mount -o remount,"
#define REMOUNT_COMMAND_RO "ro \""
#define REMOUNT_COMMAND_RW "rw \""
#define REMOUNT_COMMAND_END "\""
#define REMOUNT_COMMAND_LENGTH sizeof REMOUNT_COMMAND_START REMOUNT_COMMAND_RO REMOUNT_COMMAND_END

// Check mount state command
#define CHECK_COMMAND_START "awk '$4~/(^|,)r?($|,)/' /proc/mounts | grep -q \" "
#define CHECK_COMMAND_END " \""
#define CHECK_COMMAND_INDEX 15
#define CHECK_COMMAND_LENGTH sizeof CHECK_COMMAND_START CHECK_COMMAND_END

enum remount_mode {
	READ_ONLY,
	READ_WRITE,
	CHECK,
};

void noreturn help(void)
{
	printf(
			"Usage: remount ro|rw|ch [MOUNTPOINT]...\n"
			"Quickly manage read-only/read-write mountpoints\n"
			"\tro\tRemount read-only\n"
			"\trw\tRemount read-write\n"
			"\tch\tCheck mount state (ro/rw)\n"
			"\t--help\tPrint this help message\n"
			"MOUNTPOINT: A list of mountpoint to remount or check\n"
			"Default mountpoint list is defined in the configuration file.\n"
			"Configuration file is \"%s\". "
			"If no configuration file is found, the program uses the "
			"default configuration file located in \"%s\".\n"
			, CONFIG_FILE
			, DEFCONFIG_FILE
	      );
	exit(0);
}

void noreturn die(int error, const char *format, ...)
{
	fprintf(stderr, "Error: ");
	va_list argptr;
	va_start(argptr, format);
	vfprintf(stderr, format, argptr);
	va_end(argptr);
	fprintf(stderr, "\n");
	exit(error);
}

int main(int argc, char **argv)
{
	enum remount_mode mode;
	char *mountpoints[MOUNTPOINT_MAX_COUNT];
	int k, i;
	char buffer[BUFSIZ];

	/* Get remount mode */
	if (argc < 2)
		die(1, "Expected at least one argument, use --help to get help");

	if (strncmp(argv[1], "ro", 2) == 0)
		mode = READ_ONLY;
	else if (strncmp(argv[1], "rw", 2) == 0)
		mode = READ_WRITE;
	else if (strncmp(argv[1], "ch", 2) == 0)
		mode = CHECK;
	else if (strncmp(argv[1], "--help", 6) == 0 || strncmp(argv[1], "-h", 2) == 0)
		help();
	else
		die(1, "Unknown mode \"%s\"\nRun \"%s --help\" to get help", argv[1], argv[0]);

	/* Check permissions */
	if (mode != CHECK && geteuid() != 0)
		die(2, "The program must be run as root");
	if (mode == READ_WRITE && getuid() != 0)
		die(2, "Remounting read-write requires root permissions");


	/* Get list of mountpoint */
	k = 0;
	if (argc == 2) {
		/* Read configuration */
		FILE *config;
		if (((config = fopen(CONFIG_FILE, "r"))) == NULL && ((config = fopen(DEFCONFIG_FILE, "r")) == NULL))
			die(3, "Could not access configuration files \"%s\" and \"%s\": %s",
					CONFIG_FILE, DEFCONFIG_FILE, strerror(errno)
			   );

		/* Parse configuration */
		while (k < MOUNTPOINT_MAX_COUNT && fgets(buffer, BUFSIZ, config) != NULL) {
			// Check for comments of empty lines
			if (buffer[0] == '\0' || buffer[0] == '\n' || buffer[0] == '\r' || buffer[0] == '#')
				continue;
			if ((mountpoints[k] = malloc(MOUNTPOINT_MAX_PATH * sizeof(char))) == NULL)
				die(4, "Memory allocation error");

			// Remove trailing new line
			if ((i = strcspn(buffer, "\r\n")) >= MOUNTPOINT_MAX_PATH)
				die(3, "Path too large, maximum is %d", MOUNTPOINT_MAX_PATH);
			buffer[i] = '\0';

			strncpy(mountpoints[k], buffer, MOUNTPOINT_MAX_PATH);
			k++;
		}
	} else {
		/* Parse arguments */
		for (i = 2; i < argc && i < MOUNTPOINT_MAX_COUNT; i++) {
			if ((mountpoints[k] = malloc(MOUNTPOINT_MAX_PATH * sizeof(char))) == NULL)
				die(4, "Memory allocation error");

			strncpy(mountpoints[k], argv[i], MOUNTPOINT_MAX_PATH);
			k++;
		}
	}

	/* Check for possible overflow */
	if (k >= MOUNTPOINT_MAX_COUNT)
		die(5, "To many mountpoints, maximum is %d", MOUNTPOINT_MAX_COUNT);
	if (BUFSIZ < MAX(MOUNTPOINT_MAX_PATH + REMOUNT_COMMAND_LENGTH, MOUNTPOINT_MAX_PATH + CHECK_COMMAND_LENGTH))
		die(4, "Buffer size BUFSIZE is %d while the maxumal command size is %d, risk of overflow",
				BUFSIZ,
				MAX(MOUNTPOINT_MAX_PATH + REMOUNT_COMMAND_LENGTH, MOUNTPOINT_MAX_PATH + CHECK_COMMAND_LENGTH)
		   );

	/* Run command */
	if (mode == READ_ONLY || mode == READ_WRITE) {
		// Remount needs real UID = 0
		setuid(0);
		for (i = 0; i < k; i++) {
			strcpy(buffer, REMOUNT_COMMAND_START);
			if (mode == READ_ONLY)
				strcat(buffer, REMOUNT_COMMAND_RO);
			else if (mode == READ_WRITE)
				strcat(buffer, REMOUNT_COMMAND_RW);
			else
				die(255, "No mode selected, this should never happen");
			strcat(buffer, mountpoints[i]);
			strcat(buffer, REMOUNT_COMMAND_END);

			if (system(buffer))
				fprintf(stderr, "Warning: Could not remount \"%s\"", mountpoints[i]);
		}
	} else if (mode == CHECK) {
		int ro, rw;
		for (i = 0; i < k; i++) {
			strcpy(buffer, CHECK_COMMAND_START);
			strcat(buffer, mountpoints[i]);
			strcat(buffer, CHECK_COMMAND_END);

			// ro and rw are the exit status of the grep commands
			buffer[CHECK_COMMAND_INDEX] = 'o';
			ro = system(buffer);
			buffer[CHECK_COMMAND_INDEX] = 'w';
			if (ro != 0)
				rw = system(buffer);

			if (ro == 0)
				printf("%s is ro\n", mountpoints[i]);
			else if (rw == 0)
				printf("%s is rw\n", mountpoints[i]);
			else
				printf("%s is not mounted\n", mountpoints[i]);
		}
	}

	return 0;
}

