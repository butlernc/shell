int execute_pipe() {
	//fd[0] is for writing, fd[1] is for reading
	int fd[2];
	pipe(fd);
	
}