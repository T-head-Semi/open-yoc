
#if 0
int fsync(int fd)
{
    return 0;
}
#endif

int dup2(int oldfd, int newfd)
{
    return 0;
}

int geteuid(void)
{
    return 1;
}

int getuid(void)
{
    return 1;
}

void waitpid(void)
{
}

int pipe(int pipefd[2])
{
    return 0;
}

#if 0
void XML_ParserCreate_MM(void){}
void XML_ParserFree(void){}
void XML_SetUserData(void){}
void XML_SetElementHandler(void){}
void XML_SetCharacterDataHandler(void){}
void XML_Parse(void){}
void XML_GetErrorCode(void){}
void XML_GetCurrentLineNumber(void){}
void XML_GetCurrentColumnNumber(void){}
void XML_ErrorString(void){}
#endif
