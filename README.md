Group Members:
Anthon Worsham, Drew Swanson, Colten Stevens

4 stages complete

no known limitations outside of regular use

pipes have been implemented, no multi-piping was implemented.  Our pipe implementation starts with "pfd[2]" to create two ends of the pipe.
We check for errors and fork the code so that one child handles the write end and the other handles the read end. 
The parent then closes each end of the pipe.
