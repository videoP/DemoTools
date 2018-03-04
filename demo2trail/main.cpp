// main.cpp : Defines the entry point for the console application.
//

#include "deps.h" //only need CL_ReadDemoMessage from this?
#include "demo_utils.h"
#include "demo_common.h"
#include "utils.h"
#include "q_string.h" //added

#ifdef WIN32
#include <io.h>
#include <fcntl.h>
#endif

int main(int argc, char** argv)
{
	//printf( "Demo2Trail v" VERSION " loaded\n");
	char *demoFileName = NULL;
	char *trailFileName = NULL;

	if (argc == 2) {//Input file specified, use same name for output file 
		char *input = argv[1];

		if (COM_CompareExtension(input, ".dm_26")) { //Possible drag/drop, check extension and strip
			char temp[256]; //wtf - max path?

			//printf("Extension found\n");
			COM_StripExtension(input, temp, sizeof(temp));
			input = va("%s", temp);
		}

		demoFileName = va("%s.dm_26", input);
		trailFileName = va("%s.cfg", input);
		//printf("1Input file: %s Output file: %s\n", demoFileName, trailFileName);
		//system("pause");
	}
	else if (argc == 3) { //Input and output file specified
		char *input = argv[1];
		char *output = argv[2];

		demoFileName = va("%s.dm_26", input);
		trailFileName = va("%s.cfg", output);
		//printf("2Input file: %s Output file: %s\n", demoFileName, trailFileName);
		//system("pause");
	}
	else {
		printf("No file specified.\n"
			"Usage: \"%s\" filename outfile (optional)\n", argv[0]);
		//system("pause");
		return -1;
	}

	printf("Input file: %s Output file: %s\n", demoFileName, trailFileName);

	fileHandle_t fp;
	FS_FOpenFileRead(demoFileName, &fp, qfalse );
	if ( !fp ) {
		printf( "File %s not found.\n", demoFileName);
		//system( "pause" );
		return -1;
	}

	FILE *trailFile = fopen(trailFileName, "wb" );
	if ( !trailFile) {
		printf( "Couldn't open output file\n" );
		return -1;
	}

	qboolean demoFinished = qfalse;
	qboolean startedRace = qfalse;
	vec3_t lastOrigin = {0};
	char buf[8192] = {0};
	while ( !demoFinished ) {
		msg_t msg;
		byte msgData[ MAX_MSGLEN ];
		MSG_Init( &msg, msgData, sizeof( msgData ) );
		demoFinished = CL_ReadDemoMessage( fp, &msg ) ? qfalse : qtrue;

		if ( demoFinished ) {
			//write rest of buffer?
			//printf("stopping 1\n");
			break;
		}
		try {
			CL_ParseServerMessage( &msg );
		} catch ( int ) {
			// thrown code means it wasn't a fatal error, so we can still dump what we had
			//printf("stopping 2\n");
			break;
		}

		if ( !ctx->cl.newSnapshots ) {
			continue;
		}
		int firstServerCommand = ctx->clc.lastExecutedServerCommand;
		// process any new server commands
		for ( ; ctx->clc.lastExecutedServerCommand < ctx->clc.serverCommandSequence; ctx->clc.lastExecutedServerCommand++ ) {
			char *command = ctx->clc.serverCommands[ ctx->clc.lastExecutedServerCommand & ( MAX_RELIABLE_COMMANDS - 1 ) ];
			Cmd_TokenizeString( command );
			char *cmd = Cmd_Argv( 0 );
			if ( cmd[0] ) {
				firstServerCommand = ctx->clc.lastExecutedServerCommand;
			}
		}

		//How 2 trim neadless shit at start/end(?) of demo:

		//If tele bit was flipped, reset file?  Nvm, that will break courses with teleports.
		//If we had a timer and now we dont, reset file.  (this fucks up after we finish course)
		//If we had a timer and now we have a different timer, reset. (that fucks loopstarts?)
		//If we have a timer now, and we didnt previously, reset file (This fucks up prejumps, but oh wel?)

		//If current origin is same as previous... don't write anything...? 
		//(this fucks the timing.. but really are there any courses where people pause - besides mountain? and if so it can be done without the pause?)

		if (!startedRace && ctx->cl.snap.ps.duelTime) { //Keep track of timer resets, clear ENTIRE FILE if it happens
			startedRace = qtrue;
			Q_strncpyz(buf, "", sizeof(buf));
			fclose(fopen(trailFileName, "w"));
			//printf("Restarting race, clearing file\n");
		}
		else if (!ctx->cl.snap.ps.duelTime && startedRace) {
			startedRace = qfalse;
			//printf("Stopping race\n");
		}

		if (ctx->cl.snap.ps.duelTime) { //Only write if they are in a race
			if (!VectorCompare(ctx->cl.snap.ps.origin, lastOrigin)) //Dont write if they are standing still
			{
				char *tmpMsg = NULL;
				tmpMsg = va("%i %i %i\n", (int)(ctx->cl.snap.ps.origin[0] + 0.5f), (int)(ctx->cl.snap.ps.origin[1] + 0.5f), (int)(ctx->cl.snap.ps.origin[2] + 0.5f));

				VectorCopy(ctx->cl.snap.ps.origin, lastOrigin);
		
				if (strlen(buf) + strlen(tmpMsg) >= sizeof(buf)) { //Write to file
					fwrite(buf, strlen(buf), 1, trailFile);
					Q_strncpyz(buf, "", sizeof(buf)); //buf[0] = '\0'; ?
					//printf("Resetting buf!\n");
				}

				//printf("startedRace? %i time? %i\n", startedRace, ctx->cl.snap.ps.duelTime);

				Q_strcat(buf, sizeof(buf), tmpMsg);
			}/*
			else {
				printf("Standing still! - (%i %i %i)\n", (int)(ctx->cl.snap.ps.origin[0] + 0.5f), (int)(ctx->cl.snap.ps.origin[1] + 0.5f), (int)(ctx->cl.snap.ps.origin[2] + 0.5f));
			}
			*/
		}

	}

	//Write remaining buf to file
	fwrite(buf, strlen(buf), 1, trailFile);

	FS_FCloseFile( fp );
	fclose( trailFile );

	printf("Completed\n");

	return 0;
}

