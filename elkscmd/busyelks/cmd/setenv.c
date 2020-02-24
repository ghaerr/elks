setenv_main(argc, argv)
        char    **argv;
{
        char    buf[1024];

        strcpy(buf, argv[1]);
        if (argc > 2) {
                strcat(buf, "=");
                strcat(buf, argv[2]);
        }
        putenv(buf);
	exit(0);
}
