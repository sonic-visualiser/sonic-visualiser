/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <lo/lo.h>

void
usage(char *program_name)
{
    char *base_name = strrchr(program_name, '/');
    
    if (base_name && *(base_name + 1) != 0) {
        base_name += 1;
    } else {
        base_name = program_name;
    }

    fprintf(stderr, "\nusage: %s <OSC URL> [<values>]\n\n", program_name);
    fprintf(stderr, "example OSC URLs:\n\n"
                    "  osc.udp://localhost:19383/path/test 1.0 4.2\n"
                    "  osc.udp://my.host.org:10886/3/13/load file\n\n");
    fprintf(stderr, "numeric arguments will be treated as OSC 'f' floating point types.\n\n");
    exit(1);
}

int main(int argc, char *argv[])
{
    lo_address a;
    char *url, *host, *port, *path;
    lo_message message;
    unsigned int i;

    if (argc < 2) {
        usage(argv[0]);
        /* does not return */
    }
    url = argv[1];

    host = lo_url_get_hostname(url);
    port = lo_url_get_port(url);
    path = lo_url_get_path(url);
    a = lo_address_new(host, port);

    message = lo_message_new();

    for (i = 0; i + 2 < argc; ++i) {

	int index = i + 2;
	char *param;

	param = argv[index];
	if (!isdigit(param[0])) {
	    lo_message_add_string(message, argv[index]);
	} else {
	    lo_message_add_float(message, atof(argv[index]));
	}
    }

    lo_send_message(a, path, message);

    if (lo_address_errno(a)) {
	printf("liblo error: %s\n", lo_address_errstr(a));
    }

    free(host);
    free(port);
    free(path);

    return 0;
}

