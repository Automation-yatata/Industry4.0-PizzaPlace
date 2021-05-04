#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <postgresql/libpq-fe.h>


int main()
{	

	PGconn *conn; 
	PGresult *res;
	const char *dbconn;
	
	dbconn = "host = 'db.fe.up.pt' dbname = 'sinf2021a35' user = 'sinf2021a35' password = 'ZbnBodLV'";
	//EXAMPLE : dbconn = "host = 'db.fe.up.pt' dbname = 'sinf1920e32' user = 'sinf1920e32' password = 'QWTTIjZl'";

	conn = PQconnectdb(dbconn);
    printf("PPPPP\n");
	//PQexec(conn,"SET search_path TO testing");
	
	if (!conn){
		printf("libpq error: PQconnectdb returned NULL. \n\n");
		PQfinish(conn);
		exit(1);
	}
	
	else if (PQstatus(conn) != CONNECTION_OK){
		printf("Connection to DB failed: %s", PQerrorMessage(conn));
		PQfinish(conn);
		exit(1);
	}

	else {
		printf("Connection OK \n");
		//res = PQexec(conn, "INSERT INTO test_1920 (id, name, age) VALUES (1, 'Jane Doe', 32)");
		PQfinish(conn);
	}
	
	return 0;
	
}
