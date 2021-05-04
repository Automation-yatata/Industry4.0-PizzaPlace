#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <postgrsql/libpq-fe.h>


#define MAX_CHAR 75
#define N_MOTES 2       // motes number equals sections number
#define N_SENSOR_MOTE 5 // number of sensor per mote
#define MAX_RULES 20
#define N_ACTUATORS 10

#define BLUE "[0,0,254]"
#define GREEN "[0,254,0]"
#define RED "[254,0,0]"
#define BLACK "[0,0,0]"
#define WHITE "[255,255,255]"

long get_num_dec(int pos);
int check_message_start(void);
float get_voltage(long dec);
float get_light(long dec);
float get_current(long dec);
float get_temperature(long dec);
float get_relative_humidity(long dec);
float get_temp_humidity(float rel_hum, float temp, long dec);
int load_sensorconfig(void);
void check_OK(void);
int load_rules(int n);
void print_mote(void);
void write_2_RGB();
void outputs_update(int n_rules);
void measure_power(float **vec_sec, float **vec_hour, int moteID, float volt, float light, float curr, float temp, float humi);
void establish_DB_connection(PGconn *conn , PGresult *res ,const char *dbconn);
/*void insert_values (PGconn *conn, char *database_name, char *column_names, char *values);
void delete_values (PGconn *conn, char *database_name, char *PRIMARY_KEY, int id);
void drop_all (PGconn *conn);
void update_values (PGconn *conn, char *table_name, char *PRIMARY_KEY, int id, char *column, float value);
*/

char str[MAX_CHAR];
int count_sec;
int count_hour;

time_t old, atual;


PGconn *conn;
PGresult *res;
const char *dbconn;


typedef struct
{

    float value;
    char name[25];
    char rise[3];

} layout;

typedef struct
{

    layout pos[5];

} MOTE;

MOTE motes[N_MOTES];

typedef struct
{

    layout *sensor;
    int *out;
    char operation;
    int ref;
    int is_complex;
    char op[4];
    layout *sensor2;
    char operation2;
    int ref2;

} RULES;

RULES rules_vec[MAX_RULES];

typedef struct
{

    char name[25];
    int on;
    int off;

} OUTPUT;

OUTPUT outputs_vetor[N_ACTUATORS];
int init[N_MOTES];

long get_num_dec(int pos)
{

    int i, j, n = 5;
    char *ptr;
    char res[n];
    for (i = 0, j = pos; i < n - 1; i++, j++)
    {
        if (i == 2)
            j++;
        res[i] = str[j];
    }
    res[n - 1] = '\0';
    //printf("string:%s\n", res);
    long ex = strtol(res, &ptr, 16);
    //printf("string:%s nume:%ld\n", res, ex);
    return ex;
}

float get_voltage(long dec)
{

    return ((float)dec / 4096) * 1.5 * 2;
}
float get_light(long dec)
{

    return (0.625 * 1000000 * (((float)dec / 4096) * 1.5 / 100000) * 1000);
}
float get_current(long dec)
{

    return (0.769 * 100000 * (((float)dec / 4096) * 1.5 / 100000) * 1000);
}
float get_temperature(long dec)
{

    return (-39.6 + (0.01 * (float)dec));
}
float get_relative_humidity(long dec)
{

    return (-2.048 + (0.0367 * dec) + (-1.5955 * 0.000001 * (float)dec * (float)dec));
}
float get_temp_humidity(float rel_hum, float temp, long dec)
{

    return ((temp - 25) * (0.01 + 0.00008 * (float)dec) + rel_hum);
}

int check_message_start(void)
{
    char *token;
    char copy[MAX_CHAR];
    strcpy(copy, str);
    token = strtok(copy, " ");

    if (strcmp(token, "7E") == 0)
    {
        token = strtok(NULL, " ");

        if (strcmp(token, "45") == 0)

            return 1;

        else
            return 0;
    }
    else

        return 0;
}

void check_values(int *old_vec, int n_actuadores, int rules, int moteID)
{

    int i = 0;

    while (rules_vec[i].sensor != NULL && i < rules)
    {

        //char op [2];
        //strcpy(op, &rules_vec[i].operation);
        //printf("operation: %s\n",op);
        /*
        //printf("Name: %s\n", rules_vec[i].sensor->name);
        printf("Rise %s\n", rules_vec[i].sensor->rise);
        printf("Value %f\n", rules_vec[i].sensor->value);
        printf("Op %c\n", rules_vec[i].operation);
        printf("Ref %d\n", rules_vec[i].ref);
        //printf("Ref  %d\n",rules_vec[i].ref);
        printf("OUTPUT: %d\n", *rules_vec[i].out);
        */

        /*SUBIR
        1 - Arranca em 19 a subir até 20 - out==1 old==1 rise==1 --> mantem rise=1 ***
        2 - qd chegar a 20 out==0 old==1 rise==1  - manter o rise=1 ***
        3 - se >20 e <26 ( out==0 old==0 rise==1) - manter rise=1 ***
        4 - qd chegar a 26 (out==1 old==0 rise==1) - rise=-1
        5 - >20 (out=0 old=1 rise=-1 ) - manter o rise=-1 ***
        6 - qd chegar a 19.9 (out==1 old==0 rise==-1) - rise=1

       */
        //printf("%s %d %d\n", rules_vec[i].sensor->name, *rules_vec[i].out, old_vec[i]);
        if (*rules_vec[i].out == 1 && old_vec[i] == 0 && strcmp(rules_vec[i].sensor->rise, "1") == 0)
        {
            //printf("%s Passou de + pra -\n", rules_vec[i].sensor->name);
            strcpy(rules_vec[i].sensor->rise, "-1");
            //old_vec[i] = *rules_vec[i].out;
        }
        else if (*rules_vec[i].out == 1 && old_vec[i] == 0 && strcmp(rules_vec[i].sensor->rise, "-1") == 0)
        {
            //printf("%s Passou de - pra +\n", rules_vec[i].sensor->name);
            strcpy(rules_vec[i].sensor->rise, "1");
            //old_vec[i] = *rules_vec[i].out;
        }

        /*DESCER
        1 - Arranca em 27 até 26 - out==1 old==1 rise==-1 --> mantem rise=-1 ****
        2 - qd chegar a 26 - out==0 old==1 rise== -1 --> mantem o rise=-1 ****
        3 - se <26 e >20 - out==0 old==0 rise==-1 --> mantem o rise=-1 ***
        4 - qd chega a 20 (out==1 old==0 rise==-1) --> rise=1 
        5 - <26 (out==0 old==1 rise==1) --> rise=1 ***
        6 - qd chegar a 26.1 (out==1 old==0 rise==1) --> rise=-1

      */
        /*if (*rules_vec[i].out == 1 && old_vec[i] == 0 && strcmp(rules_vec[i].sensor->rise, "-1") == 0)
        {
            strcpy(rules_vec[i].sensor->rise, "1");
        }
        else if (*rules_vec[i].out == 1 && old_vec[i] == 0 && strcmp(rules_vec[i].sensor->rise, "1") == 0)
        {
            strcpy(rules_vec[i].sensor->rise, "-1");
        }
    */
        i++;
    }

    i = 0;
    while (i < rules)
    {
        char aux[2];
        sprintf(aux, "%d", moteID);

        if (strstr(rules_vec[i].sensor->name, aux) != NULL)
        {
            old_vec[i] = *rules_vec[i].out;
        }
        i++;
    }

    /*for (i = 0; i < n_actuadores; i++)
    {
        printf("%s ON:%d OFF:%d\n", outputs_vetor[i].name, outputs_vetor[i].on, outputs_vetor[i].off);
    }*/
}

void new_values(int moteID)
{

    //change variables in file
    FILE *f_msgcreator;
    FILE *f_sensorconfig;
    char frases[N_MOTES][300];
    char frase[300];
    char *token;
    int numero_motes = 0;
    int n_sensores = 0;

    if (moteID == 1)
    {

        f_msgcreator = fopen("MsgCreatorConf.txt", "r");
    }
    else if (moteID == 2)
    {

        f_msgcreator = fopen("msg2/MsgCreatorConf.txt", "r");
    }

    if (f_msgcreator == NULL)
    {

        printf("Error new_values 2\n");
        exit(EXIT_FAILURE);
    }

    fgets(frase, 300, f_msgcreator);

    token = strtok(frase, "[");
    token = strtok(NULL, "[");
    char aux[10];
    strcpy(aux, token);

    //printf("token i: %s\n",token);
    n_sensores = 0;
    token = (strtok(token, ","));

    if (token != NULL)
    {

        //mais do que 1 sensor
        while (token != NULL)
        {

            n_sensores++;
            //printf("token: %s\n",token);
            token = (strtok(NULL, ","));
        }
    }
    else
    {

        n_sensores = 1;
    }

    //printf("%d\n",n_sensores);
    char sensores[n_sensores][2];

    int i = 0;
    while (i < n_sensores)
    {

        if (n_sensores == 1)
        {
            token = (strtok(aux, "]"));
            strcpy(sensores[i], token);
            break;
        }

        //printf("%s\n",aux);
        if (i == 0)
        {
            token = (strtok(aux, ","));
        }
        else if (i == n_sensores - 1)
        {
            token = strtok(NULL, "]");
        }
        else
        {
            token = (strtok(NULL, ","));
        }
        strcpy(sensores[i], token);
        //printf("sensor: %s\n",sensores[i]);
        i++;
    }

    //////////////////////////////////////ordem dos sensores acima

    i = 0;
    char final_string[300];

    fseek(f_msgcreator, 0, SEEK_SET);
    fgets(frase, 300, f_msgcreator);
    char frase2[300];
    fseek(f_msgcreator, 0, SEEK_SET);
    fgets(frase2, 300, f_msgcreator);

    char *identifier1;
    char *identifier2;

    final_string[0] = '\0';
    //printf("inicio: %s\n",final_string);
    token = strtok_r(frase, "[", &identifier1);
    //printf("token 1: %s\n",token);
    strcat(final_string, token);
    //printf("final: %s\n",final_string);

    token = strtok_r(NULL, "[", &identifier1);
    //printf("token 2: %s\n",token);
    strcat(final_string, "[");
    strcat(final_string, token);
    strcat(final_string, "[");
    //printf("final2: %s\n",final_string);
    int posi;

    //printf("Inicio1\n");
    while (i < n_sensores)
    {

        token = strtok_r(NULL, "[", &identifier1);

        if (strcmp(sensores[i], "1") == 0)
        {

            int ii = 0;
            //printf("%s\n",motes[moteID-1].pos[ii].name);
            //printf("rise: %s\n",motes[moteID-1].pos[ii].rise);
            while (motes[moteID].pos[ii].name != NULL)
            {

                //printf("mote: %s\n",motes[moteID-1].pos[ii].name);
                if (strstr(motes[moteID - 1].pos[ii].name, "LIGHT") != NULL)
                {
                    //printf("id: %d\n",moteID-1);
                    //printf("mote: %s\n",motes[moteID-1].pos[ii].name);

                    posi = ii;

                    break;
                }
                ii++;
                //printf("ii2: %d\n",ii);
                //printf("mote2: %s\n",motes[moteID-1].pos[ii].name);
            }
            //printf("%d\n",posi);
            token = strtok_r(token, ",", &identifier2);
            strcat(final_string, "[");
            strcat(final_string, token);
            strcat(final_string, ",");
            token = strtok_r(NULL, ",", &identifier2);
            strcat(final_string, token);
            strcat(final_string, ",");
            token = strtok_r(NULL, ",", &identifier2);
            strcat(final_string, token);
            strcat(final_string, ",");
            //printf("In New Values\n");
            //printf("Mote Name %s\n", motes[moteID - 1].pos[posi].name);
            //printf("Mote RIse %s\n", motes[moteID - 1].pos[posi].rise);
            if (init[moteID - 1] == 1)
            {
                char *identifier3;
                token = strtok_r(NULL, ",", &identifier2);
                token = strtok_r(token, "]", &identifier3);
                strcpy(motes[moteID - 1].pos[posi].rise, token);
            }

            strcat(final_string, motes[moteID - 1].pos[posi].rise);
            if (i == n_sensores - 1)
            {
                strcat(final_string, "]]");
            }
            else
            {
                strcat(final_string, "],");
            }
            //printf("L-> %s\n",final_string);
        }
        else if (strcmp(sensores[i], "3") == 0)
        {

            int ii = 0;
            //printf("%s\n",motes[moteID-1].pos[ii].name);
            //printf("rise: %s\n",motes[moteID-1].pos[ii].rise);
            //strncmp(motes[moteID].pos[ii].name,"\0",3)
            while (motes[moteID].pos[ii].name != NULL)
            {

                if (strstr(motes[moteID - 1].pos[ii].name, "TEMP") != NULL)
                {
                    //printf("id: %d\n",moteID-1);
                    //printf("mote: %s\n",motes[moteID-1].pos[ii].name);

                    posi = ii;
                    //printf("%s\n",motes[moteID-1].pos[ii].name);
                    break;
                }
                ii++;
            }
            //printf("%d\n",posi);
            token = strtok_r(token, ",", &identifier2);
            strcat(final_string, "[");
            strcat(final_string, token);
            strcat(final_string, ",");
            token = strtok_r(NULL, ",", &identifier2);
            strcat(final_string, token);
            strcat(final_string, ",");
            token = strtok_r(NULL, ",", &identifier2);
            strcat(final_string, token);
            strcat(final_string, ",");
            //printf("In New Values\n");
            //printf("Mote Name %s\n", motes[moteID - 1].pos[posi].name);
            //printf("MOte RIse %s\n", motes[moteID - 1].pos[posi].rise);
            if (init[moteID - 1] == 1)
            {
                char *identifier3;
                token = strtok_r(NULL, ",", &identifier2);
                token = strtok_r(token, "]", &identifier3);
                strcpy(motes[moteID - 1].pos[posi].rise, token);
                //printf("%s\n", token);
            }
            strcat(final_string, motes[moteID - 1].pos[posi].rise);
            if (i == n_sensores - 1)
            {
                strcat(final_string, "]] ");
            }
            else
            {
                strcat(final_string, "],");
            }
            //printf("in function new\n");
            //printf("id: %d\n",moteID);
            //printf("grandeza: %s\n",motes[moteID-1].pos[posi].name);
            //printf("rise temp:%s\n",motes[moteID-1].pos[posi].rise);
            //printf("%s\n",final_string);
        }
        else if (strcmp(sensores[i], "4") == 0)
        {
            int ii = 0;

            //printf("%s\n",motes[moteID-1].pos[ii].name);
            //printf("rise: %s\n",motes[moteID-1].pos[ii].rise);
            while (motes[moteID].pos[ii].name != NULL)
            {

                if (strstr(motes[moteID - 1].pos[ii].name, "HU") != NULL)
                {
                    //printf("id: %d\n",moteID-1);

                    posi = ii;

                    break;
                }
                ii++;
            }
            //printf("%d\n",posi);
            token = strtok_r(token, ",", &identifier2);
            strcat(final_string, "[");
            strcat(final_string, token);
            strcat(final_string, ",");
            token = strtok_r(NULL, ",", &identifier2);
            strcat(final_string, token);
            strcat(final_string, ",");
            token = strtok_r(NULL, ",", &identifier2);
            strcat(final_string, token);
            strcat(final_string, ",");
            //printf("2token %s\n",token);
            //printf("In New Values\n");
            // printf("Mote Name %s\n", motes[moteID - 1].pos[posi].name);
            //printf("MOte RIse %s\n", motes[moteID - 1].pos[posi].rise);
            if (init[moteID - 1] == 1)
            {
                char *identifier3;
                token = strtok_r(NULL, ",", &identifier2);
                token = strtok_r(token, "]", &identifier3);
                strcpy(motes[moteID - 1].pos[posi].rise, token);
            }
            strcat(final_string, motes[moteID - 1].pos[posi].rise);
            if (i == n_sensores - 1)
            {
                strcat(final_string, "]]");
            }
            else
            {
                strcat(final_string, "],");
            }
            //printf("%s\n",final_string);
        }
        else
        {

            strcat(final_string, "[");
            strcat(final_string, token);
        }
        i++;
    }

    //printf("finaly %s\n",final_string);
    fclose(f_msgcreator);

    if (moteID == 1)
    {

        f_msgcreator = fopen("MsgCreatorConf.txt", "w");
        strcat(final_string, " -i 1");
    }
    else if (moteID == 2)
    {

        f_msgcreator = fopen("msg2/MsgCreatorConf.txt", "w");
        strcat(final_string, " -i 2");
    }

    if (f_msgcreator == NULL)
    {

        printf("Error new_values 3\n");
        exit(EXIT_FAILURE);
    }

    // printf("finaly2 %s\n", final_string);
    //fseek(f_msgcreator,0,SEEK_SET);
    fputs(final_string, f_msgcreator);
    //fputs(final_string,f_msgcreator);
    init[moteID - 1] = 0;
    final_string[0] = '\0';
    fclose(f_msgcreator);
}

int load_sensorconfig(void)
{

    FILE *f;

    char line[2 * MAX_CHAR];
    char *token;
    char dec_to_str[3];
    char inputs[MAX_CHAR];
    char outputs[MAX_CHAR];
    int i = 0, j = 0, cnt = 0;

    f = fopen("SensorConfigurations.txt", "r");
    if (f == NULL)
    {
        printf("ERROR OPENING SensorConfig.txt\n");
    }

    while (1)
    {

        if (fgets(line, 2 * MAX_CHAR, f) != NULL)
        {
            //printf("%s\n", line);
            token = strtok(line, ":");
            //sprintf(dec_to_str, "%d", i + 1);
            strcpy(dec_to_str,&token[strlen(token)-1]);
            char insert_tb_section[3];
            
            //DB INSERT table: SECTION
            // Só insere se for nao existir

            strcpy(insert_tb_section,dec_to_str);
            //printf("SECTION %s\n",insert_tb_section);
            //insert_values(conn,"section","___section_id___",insert_tb_section);
            ///////

            while (1)
            {
                if (strstr(token, dec_to_str) != NULL)
                {
                    token = strtok(NULL, ":");
                    strcpy(inputs, token);

                    token = strtok(NULL, "\n");
                    strcpy(outputs, token);

                    break;
                }
                /*else
                {
                    printf("ENTROU");
                    i++;
                    sprintf(dec_to_str, "%d", i + 1);
                }*/
            }

            puts(inputs);puts(outputs);

            // write inputs on mote[]
            token = strtok(inputs, ",");

            // Check mote_id
            char insert_tb_mote[5];

            strcpy(insert_tb_mote, &token[strlen(token)-1]);
            //printf("%s\n",insert_tb_mote);
            strcat(insert_tb_mote,"|");
            strcat(insert_tb_mote,insert_tb_section);
            //printf("MOTE  %s\n",insert_tb_mote);


            //DB INSERT table: MOTE
            // Só insere se nao existir
            
            //insert_values(conn,"mote","mote_id,section_id",insert_tb_mote);

            char mote_id[2];
            strncpy(mote_id,insert_tb_mote,1);
            mote_id[1]='\0';
            i=atoi(insert_tb_section)-1;
            while (token != NULL)
            {
                // DB INSERT table: SENSOR
                // Só insere se nao existir

                char insert_tb_sensor[50];
                
                strcpy(insert_tb_sensor,token);
                strcat(insert_tb_sensor,"|");
                strcat(insert_tb_sensor,mote_id);
                strcat(insert_tb_sensor,"|");
                strcat(insert_tb_sensor,"0");
                //printf("SENSOR %s\n",insert_tb_sensor);
                //insert_values(conn,"sensor","name,mote_id,actual_value",insert_tb_sensor);
                printf("sensor:%s  valor de i:%d\n",token,i);
                strcpy(motes[i].pos[j].name, token);
                printf("teste:%s\n",motes[i].pos[j].name);
                j++;
                token = strtok(NULL, ",");
            }
            j = 0;

            // write outputs on outputs_vector[]
            token = strtok(outputs, ",");

            

            while (token != NULL && cnt < N_ACTUATORS)
            {
                
                // DB INSERT table: ACTUATOR
                // Só insere se nao existir

                char insert_tb_actuator[50];

                strcpy(insert_tb_actuator,token);
                strcat(insert_tb_actuator,"|");
                strcat(insert_tb_actuator,mote_id);
                strcat(insert_tb_actuator,"|");
                strcat(insert_tb_actuator,"0");
                //printf("ACTUATOR %s\n",insert_tb_actuator);
                //insert_values(conn,"actuator","name,mote_id,actual_state",insert_tb_actuator);

                strcpy(outputs_vetor[cnt].name, token);
                cnt++;
                token = strtok(NULL, ",");
            }
        }
        else
        {
            check_OK();
            //printf("\n");
            break;
        }

    }

    fclose(f);
    return cnt;
}

void check_OK(void)
{

    printf("Vetor de MOTES\n");
    for (int i = 0; i < 2; i++)
    {
        for (int j = 0; j < 5; j++)
        {
            printf("%s ", motes[i].pos[j].name);
        }

        printf("\n");
    }

    printf("Vetor de Saidas\n");
    for (int i = 0; i < N_ACTUATORS; i++)
    {
        printf("%s ", outputs_vetor[i].name);
    }
    printf("\n");
    return;
}
int load_rules(int n)
{

    FILE *f;
    char line[2 * MAX_CHAR];
    int i = 1, k = 0, j = 0;
    f = fopen("SensorRules.txt", "r");
    if (f == NULL)
    {
        printf("ERROR OPENING SensorRules.txt\n");
    }
    char *token;
    char dec_to_str[3];
    char subject[MAX_CHAR];
    char subject2[MAX_CHAR];
    char predicate[MAX_CHAR];

    while (fgets(line, 2 * MAX_CHAR, f) != NULL && k < MAX_RULES)
    {
        //printf("%s\n", line);
        j = 0;
        int is_and = 0, is_or = 0;

        if (strstr(line, "AND ") != NULL)
            is_and = 1;
        if (strstr(line, "OR ") != NULL)
            is_or = 1;

        token = strtok(line, ":");
        //puts(token);

        // i related to N_MOTES
        // j related to pos[] in layout struct (whats sensor; 0(volt) etc)
        // k  related to position in RULES vector
        sprintf(dec_to_str, "%d", i);
        printf("://%s\n", dec_to_str);

        while (i <= N_MOTES)
        {
            if (strstr(token, dec_to_str) != NULL)
            {
                printf("token %s\n",token);
                token = strtok(NULL, " ");
                printf("token %s\n",token);

                if (strcpy(subject, token) == NULL)
                {
                    printf("ERROR on load_rules subject\n");
                    fclose(f);
                    return -1;
                }

                //puts(token);

                if (is_and == 0 && is_or == 0)
                {

                    token = strtok(NULL, "\0");

                    //puts(token);

                    if (strcpy(predicate, token) == NULL)
                    {
                        printf("ERROR on load_rules predicate(1)\n");
                        fclose(f);
                        return -1;
                    }
                    puts(subject);puts(predicate);
                    break;
                }
                else
                {

                    token = strtok(NULL, " ");

                    //puts(token);


                    token = strtok(NULL, " ");

                    //puts(token);

                    if (strcpy(subject2, token) == NULL)
                    {

                        printf("ERROR on load_rules subject2\n");
                        fclose(f);
                        return -1;
                    }

                    token = strtok(NULL, "\0");

                    //puts(token);

                    if (strcpy(predicate, token) == NULL)
                    {

                        printf("ERROR on load_rules predicate(2)\n");
                        fclose(f);
                        return -1;
                    }

                    //puts(subject);
                    //printf("AND?%d\n",is_and);
                        char Op_between_rules[4];
                        if(is_and==1) strcpy(Op_between_rules, "AND");
                        else if (is_and==0) strcpy(Op_between_rules, "OR");

                        char insert_tb_op_r_subr[50];

                        strcpy(insert_tb_op_r_subr, "DEFAULT");
                        strcat(insert_tb_op_r_subr, "|");
                        strcat(insert_tb_op_r_subr, Op_between_rules);
                        strcat(insert_tb_op_r_subr, "|");
                        strcat(insert_tb_op_r_subr, "DEFAULT");
                        strcat(insert_tb_op_r_subr, "|");
                        strcat(insert_tb_op_r_subr, "0");

                        //insert_values(conn,"op_r_subr","subrule_id,op_between_rules,rule_id",insert_tb_op_r_subr);


                    //puts(subject2);
                    //puts(predicate);
                    break;
                }
            }
            else
            {
                printf("ENTROU\n");
                printf("i value:%d\n",i);
                i++;
                dec_to_str[0]='\0';
                sprintf(dec_to_str, "%d", i);
            }
            //printf("Sucess Mote:%d\n", i );
        }

        int true_i = i;
        i = 0;
        j = 0;

        while (1)
        {
            // Load 1st part --> Sensor;Oper;Ref
            //puts(subject);
            //puts(predicate);
            //printf("%s\n",motes[i].pos[j].name);
            //printf("STRSTR__%s\n",strstr(subject, motes[i].pos[j].name));

            if (strstr(subject, motes[i].pos[j].name) != NULL)
            {
                rules_vec[k].sensor = &motes[i].pos[j];
                rules_vec[k].operation = subject[strlen(motes[i].pos[j].name)];
                //printf("%c\n", rules_vec[k].operation);
                    char operation1[2];
                    strcpy(operation1, &rules_vec[k].operation);
                    //printf("%d\n", rules_vec[k].ref);
                sscanf(&subject[strlen(motes[i].pos[j].name) + 1], "%d", &rules_vec[k].ref);
                //printf("%d\n", rules_vec[k].ref);
                printf("%s\n", motes[i].pos[j].name);

                    char ref_value1[4];
                    sprintf(ref_value1, "%d", rules_vec[k].ref);

                    char SENSOR_NAME1[7];
                    strcpy(SENSOR_NAME1, motes[i].pos[j].name);


                    char insert_tb_subrule[50];
                    strcpy(insert_tb_subrule, "DEFAULT");
                    strcat(insert_tb_subrule, "|");
                    strcat(insert_tb_subrule, operation1);
                    strcat(insert_tb_subrule, "|");
                    strcat(insert_tb_subrule, ref_value1);
                    strcat(insert_tb_subrule, "|");
                    strcat(insert_tb_subrule, SENSOR_NAME1);
                    strcat(insert_tb_subrule, "|");
                    strcat(insert_tb_subrule, "0");

                    //insert_values(conn,"subrule","subrule_id,operation,ref_value,name",insert_tb_subrule);

                if (is_and == 1 || is_or == 1)
                {

                    rules_vec[k].is_complex = 1;
                    if (is_and == 1)
                    {
                        strcpy(rules_vec[k].op, "AND");
                        is_and = 0;
                        break;
                    }
                    else
                    {
                        strcpy(rules_vec[k].op, "OR");
                        is_or = 0;
                        break;
                    }
                }
                else
                {
                    break;
                }
            }
            else
            {
                if (strncmp(motes[i].pos[j + 1].name, "\0", 3))
                {
                    j++;
                }
                else
                {
                    i++;
                    j = 0;
                }
                //printf("j:%d i:%d\n",j,i);

                if (i == N_MOTES)
                {
                    printf("Error on detect sensor in mote %d\n", i + 1);
                    fclose(f);
                    return -1;
                }
            }
        }

        j = 0;
        i = 0;
        while (1)
        {
            if (rules_vec[k].is_complex == 0)
                break;
            // Load 2nd part --> Sensor2;Oper2;Ref2
            //puts(subject2);

            //printf("%s\n",motes[i].pos[j].name);
            if (strstr(subject2, motes[i].pos[j].name) != NULL)
            {
                rules_vec[k].sensor2 = &motes[i].pos[j];
                rules_vec[k].operation2 = subject2[strlen(motes[i].pos[j].name)];
                //printf("%c\n", rules_vec[k].operation2);
                    char operation2[2];
                    strcpy(operation2, &rules_vec[k].operation2);

                sscanf(&subject2[strlen(motes[i].pos[j].name) + 1], "%d", &rules_vec[k].ref2);

                char ref_value2[4];
                    sprintf(ref_value2, "%d", rules_vec[k].ref2);

                    char SENSOR_NAME2[7];
                    strcpy(SENSOR_NAME2, motes[i].pos[j].name);


                    char insert_tb_subrule[50];
                    strcpy(insert_tb_subrule, "DEFAULT");
                    strcat(insert_tb_subrule, "|");
                    strcat(insert_tb_subrule, operation2);
                    strcat(insert_tb_subrule, "|");
                    strcat(insert_tb_subrule, ref_value2);
                    strcat(insert_tb_subrule, "|");
                    strcat(insert_tb_subrule, SENSOR_NAME2);
                    strcat(insert_tb_subrule, "|");
                    strcat(insert_tb_subrule, "0");

                    //insert_values(conn,"subrule","subrule_id,operation,ref_value,name",insert_tb_subrule);
                break;
            }
            else
            {
                if (strncmp(motes[i].pos[j + 1].name, "\0", 3))
                {
                    j++;
                }
                else
                {
                    i++;
                    j = 0;
                }
                //printf("j:%d i:%d\n",j,i);

                if (i == N_MOTES)
                {
                    printf("Error on detect sensor in mote %d\n", i + 1);
                    fclose(f);
                    return -1;
                }
            }
        }
        i = true_i;
        j = 0;

        //Load 2nd part (predicate) --> Actuator; Cond:ON/OFF

        while (j < n)
        {
            if (!strncmp(outputs_vetor[j].name, "\0", 3))
                break;

            if (strstr(predicate, outputs_vetor[j].name) != NULL)
            {
                 //printf("%s\n",predicate);
                 //printf("%s\n",outputs_vetor[j].name);
                    char ACTUATOR_NAME[20];
                    strcpy(ACTUATOR_NAME, outputs_vetor[j].name);

                    char insert_tb_rule[50];

                    strcpy(insert_tb_rule, "DEFAULT");
                    strcat(insert_tb_rule, "|");
                    strcat(insert_tb_rule, "0");

                    //insert_values(conn,"rule","rule_id,name",insert_tb_rule);


                char *token = strtok(predicate, ":");

                //puts(token);
                token = strtok(NULL, ":");
                //puts(token);
                if (token == NULL)
                {
                    break;
                }
                // If ON, then we know that when the rules is verified(rgb_matrix_write=1), we need to
                // turn ON the actuator

                if (strstr(token, "ON ") != NULL)
                {
                    rules_vec[k].out = &outputs_vetor[j].on;

                    k++;
                    break;
                }
                else
                {
                    // If OFF, then we know that when the rules is verified(rgb_matrix_write=1), we need to
                    // turn OFF the actuator
                    if (strstr(token, "OFF ") != NULL)
                    {
                        rules_vec[k].out = &outputs_vetor[j].off;
                        k++;
                        break;
                    }
                    else
                    {
                        printf("ERROR on detect actuator!\n");
                        fclose(f);
                        return -1;
                    }
                }
            }
            else
            {
                j++;
            }
        }

        i = 1;
    }

    i = 0;

    while (i < k)
    {
        if(rules_vec[i].is_complex==0){
        printf("%s_%c_%d || Actuator ON|OFF %d\n", rules_vec[i].sensor->name,
               rules_vec[i].operation, rules_vec[i].ref, *rules_vec[i].out);
        i++;
        }else
        {
            printf("%s_%c_%d %s %s_%c_%d || Actuator ON|OFF %d\n", rules_vec[i].sensor->name,
               rules_vec[i].operation, rules_vec[i].ref,rules_vec[i].op,rules_vec[i].sensor2->name,
               rules_vec[i].operation2, rules_vec[i].ref2, *rules_vec[i].out);
            i++;
        }
    }
    fclose(f);
    return k;
}
void print_mote(void)
{

    for (int i = 0; i < N_MOTES; i++)
    {
        int j = 0;
        printf("[%d] %s:%.2f %s:%.2f %s:%.2f %s:%.2f %s:%.2f\n", i + 1, motes[i].pos[j].name,
               motes[i].pos[j].value, motes[i].pos[j + 1].name, motes[i].pos[j + 1].value, motes[i].pos[j + 2].name, motes[i].pos[j + 2].value, motes[i].pos[j + 3].name, motes[i].pos[j + 3].value, motes[i].pos[j + 4].name, motes[i].pos[j + 4].value);
    }
}

void write_2_RGB()
{

    const char *channelRGB = "/tmp/ttyV13";
    FILE *matrix_channel;

    matrix_channel = fopen(channelRGB, "w");

    int n_atuadores = 0;
    for (int i = 0; i < N_ACTUATORS; i++)
    {
        if (!strncmp(outputs_vetor[i].name, "\0", 3))
            break;
        else
            n_atuadores++;
        //printf("%s", outputs_vetor[i].name);
    }
    //printf("n: %d\n",n_atuadores);

    int n_seccões = N_MOTES;

    int celulas_matriz = n_atuadores + n_seccões - 1;

    FILE *f;
    f = fopen("RGBMatrixConf.txt", "w");
    if (f == NULL)
    {
        printf("Error opening RGBMatrixConfig.txt\n");
    }

    FILE *fp;
    fp = fopen("matrix.txt", "w");
    if (fp == NULL)
    {
        printf("Error opening matrix.txt\n");
    }

    fprintf(f, "-a %i -b 50", celulas_matriz);
    //fprintf(matrix_channel, "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s
    //,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s",
    //"["BLACK,BLACK,BLACK,BLACK,BLACK,BLACK,BLACK,BLACK,BLACK,BLACK,GREEN,GREEN,GREEN,GREEN,BLACK,BLACK,BLACK,BLACK,BLUE,BLUE,BLUE,BLUE,BLACK,BLACK,BLACK,BLACK,GREEN,GREEN,GREEN,GREEN,BLACK,BLACK
    //,BLACK,BLACK,BLACK,BLACK,BLACK,BLACK,BLACK,BLACK,BLACK,BLACK,GREEN,GREEN,GREEN,GREEN,BLACK,BLACK,BLACK,BLACK,BLACK,BLACK,BLACK,BLACK,BLACK,BLACK,BLACK,BLACK,BLACK,BLACK,BLACK,BLACK,BLACK,BLACK "]\n");

    /*char *ATUATOR= malloc(sizeof(char)*1024);

    if (outputs_vetor[0].on==1 || outputs_vetor[0].off==1){
        strcpy ( ATUATOR,"[255,255,0]");
    }
    else strcpy ( ATUATOR,"[255,255,255]");
    */
    //fprintf(matrix_channel, "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s", "["BLACK,GREEN,GREEN,ATUATOR,ATUATOR,BLACK,BLACK,BLUE,BLUE,BLUE,BLUE,BLACK,BLACK,RED,RED,RED,RED,BLACK,BLACK,BLACK,BLACK,BLACK,BLACK,BLACK,BLACK,GREEN,GREEN,GREEN,GREEN,BLACK,BLACK,BLACK,BLACK,BLACK,BLACK,BLACK"]\n");

    int par;

    if (celulas_matriz % 2 == 0)
        par = 1;
    else
        par = 0;

    int seccao_atual = 1;
    char *str = malloc(sizeof(char) * 1024);
    sprintf(str, "%d", seccao_atual);

    for (int i = 0; i < n_atuadores; i++)
    {
        if (!strncmp(outputs_vetor[i].name, "\0", 3))
            break;

        //printf("teste");
        //printf("%s %s \n", outputs_vetor[i].name, outputs_vetor[i+1].name);
        //if (strstr(str, outputs_vetor[i+1].name)==NULL) printf("teste1\n");
        //if (strstr(str, outputs_vetor[i].name)!=NULL) printf("teste2\n");
        //printf("%s\n", str);
        if (strstr(outputs_vetor[i].name, str) != NULL && strstr(outputs_vetor[i + 1].name, str) == NULL && strncmp(outputs_vetor[i].name, "\0", 3))
        {

            for (int j = N_ACTUATORS; j > i + 1; j--)
            {
                //printf("i=%i; j=%i\n", i,j);
                if (!strncmp(outputs_vetor[j - 1].name, "\0", 3))
                    continue;
                outputs_vetor[j] = outputs_vetor[j - 1];
                if (j == i + 2)
                {
                    strcpy(outputs_vetor[j - 1].name, "Skipped");
                    //printf("Skipped");
                    outputs_vetor[j - 1].on = 0;
                    outputs_vetor[j - 1].off = 0;
                }
            }
            seccao_atual++;
            sprintf(str, "%d", seccao_atual);
        }
    }

    if (par == 0 && celulas_matriz != 3)
    {
        fprintf(fp, "[");

        for (int i = 0; i < celulas_matriz; i++)
        {

            for (int j = 0; j < ((celulas_matriz - celulas_matriz / 2) / 2) + 1; j++)
            {
                //printf("j=%i",j);
                fprintf(fp, "%s,", BLACK);
            }

            for (int k = 0; k < 2; k++)
            {

                if (strcmp(outputs_vetor[i].name, "Skipped") == 0)
                    fprintf(fp, "%s,", BLACK);

                else if (outputs_vetor[i].on == 1)
                    fprintf(fp, "%s,", GREEN);
                else if (outputs_vetor[i].off == 1)
                    fprintf(fp, "%s,", RED);
                else
                    fprintf(fp, "%s,", WHITE);
            }

            for (int l = 0; l < ((celulas_matriz - celulas_matriz / 2) / 2); l++)
            {
                if (l == ((celulas_matriz - celulas_matriz / 2) / 2) - 1 && i == celulas_matriz - 1)
                    fprintf(fp, "%s", BLACK);
                else
                    fprintf(fp, "%s,", BLACK);
            }
        }
        fprintf(fp, "]\n");
    }

    if (celulas_matriz == 3)
    {
        fprintf(fp, "[");

        for (int i = 0; i < celulas_matriz; i++)
        {

            for (int j = 0; j < 1; j++)
            {
                //printf("j=%i",j);
                fprintf(fp, "%s,", BLACK);
            }

            for (int k = 0; k < 2; k++)
            {

                if (strcmp(outputs_vetor[i].name, "Skipped") == 0 && (i != celulas_matriz - 1 || k == 1))
                    fprintf(fp, "%s,", BLACK);
                else if (strcmp(outputs_vetor[i].name, "Skipped") == 0 && i == celulas_matriz - 1 && k == 1)
                    fprintf(fp, "%s", BLACK);

                else if (outputs_vetor[i].on == 1 && i == celulas_matriz - 1 && k == 1)
                    fprintf(fp, "%s", GREEN);
                else if (outputs_vetor[i].on == 1)
                    fprintf(fp, "%s,", GREEN);

                else if (outputs_vetor[i].off == 1 && i == celulas_matriz - 1 && k == 1)
                    fprintf(fp, "%s", RED);
                else if (outputs_vetor[i].off == 1)
                    fprintf(fp, "%s,", RED);
                else
                    fprintf(fp, "%s,", WHITE);
            }
        }
        fprintf(fp, "]\n");
    }

    if (par == 1)
    {

        int j_limite;
        if ((celulas_matriz / 2) % 2 == 0)
            j_limite = ((celulas_matriz - celulas_matriz / 2) / 2);
        else
            j_limite = ((celulas_matriz - celulas_matriz / 2) / 2) + 1;

        fprintf(fp, "[");

        for (int i = 0; i < celulas_matriz; i++)
        {

            for (int j = 0; j < j_limite; j++)
            {

                //printf("j=%i",j);
                fprintf(fp, "%s,", BLACK);
            }

            for (int k = 0; k < 2; k++)
            {

                if (strcmp(outputs_vetor[i].name, "Skipped") == 0)
                    fprintf(fp, "%s,", BLACK);

                else if (outputs_vetor[i].on == 1)
                    fprintf(fp, "%s,", GREEN);
                else if (outputs_vetor[i].off == 1)
                    fprintf(fp, "%s,", RED);
                else
                    fprintf(fp, "%s,", WHITE);
            }

            for (int l = 0; l < j_limite; l++)
            {
                if (l == ((celulas_matriz - celulas_matriz / 2) / 2) && i == celulas_matriz - 1)
                    fprintf(fp, "%s", BLACK);
                else
                    fprintf(fp, "%s,", BLACK);
            }
        }
        fprintf(fp, "]\n");
    }

    fclose(fp);

    for (int i = 0; i < N_ACTUATORS * 2; i++)
    {
        //printf("i=%i; j=%i\n", i,j);
        if (!strncmp(outputs_vetor[i].name, "\0", 3))
            break;

        if (strcmp(outputs_vetor[i].name, "Skipped") == 0)
        {
            for (int j = i; j < N_ACTUATORS; j++)
            {
                outputs_vetor[j] = outputs_vetor[j + 1];
            }
        }
    }

    char *file_contents;
    long input_file_size;
    FILE *input_file = fopen("matrix.txt", "rb");
    fseek(input_file, 0, SEEK_END);
    input_file_size = ftell(input_file);
    rewind(input_file);
    file_contents = malloc((input_file_size + 1) * (sizeof(char)));
    fread(file_contents, sizeof(char), input_file_size, input_file);
    fclose(input_file);
    file_contents[input_file_size] = 0;

    fprintf(matrix_channel, "%s", file_contents);
    //printf("%s", file_contents);
    /*char ch;

    ch = fgetc(fp);
        while (ch != EOF)
        {
            
            fputc(ch, matrix_channel);

            
            ch = fgetc(fp);
        }
        */

    for (int i = 0; i < N_ACTUATORS; i++)
    {

        outputs_vetor[i].on = 0;
        outputs_vetor[i].off = 0;
    }

    //free(ATUATOR);
    fclose(matrix_channel);
    fclose(f);
    free(str);
}

void outputs_update(int n_rules)
{

    for (int i = 0; i < n_rules; i++)
    {

        if (rules_vec[i].operation == 0)
        {
            break;
        }
        else if (rules_vec[i].is_complex == 0)
        {

            if (!strcmp(&rules_vec[i].operation, ">") && rules_vec[i].sensor->value > rules_vec[i].ref)
            {
                *rules_vec[i].out = 1;
            }

            else if (!strcmp(&rules_vec[i].operation, "<") && rules_vec[i].sensor->value < rules_vec[i].ref)
            {
                *rules_vec[i].out = 1;
            }
        }
        else
        {
            if (!strcmp(&rules_vec[i].operation, ">") && rules_vec[i].sensor->value > rules_vec[i].ref && !strcmp(&rules_vec[i].operation2, ">") && rules_vec[i].sensor2->value > rules_vec[i].ref2 && strstr(rules_vec[i].op, "AND") != NULL)
            {
                *rules_vec[i].out = 1;
            }
            else if (!strcmp(&rules_vec[i].operation, ">") && rules_vec[i].sensor->value > rules_vec[i].ref && !strcmp(&rules_vec[i].operation2, "<") && rules_vec[i].sensor2->value < rules_vec[i].ref2 && strstr(rules_vec[i].op, "AND") != NULL)
            {
                *rules_vec[i].out = 1;
            }
            else if (!strcmp(&rules_vec[i].operation, "<") && rules_vec[i].sensor->value < rules_vec[i].ref && !strcmp(&rules_vec[i].operation2, "<") && rules_vec[i].sensor2->value < rules_vec[i].ref2 && strstr(rules_vec[i].op, "AND") != NULL)
            {
                *rules_vec[i].out = 1;
            }
            else if (!strcmp(&rules_vec[i].operation, "<") && rules_vec[i].sensor->value < rules_vec[i].ref && !strcmp(&rules_vec[i].operation2, ">") && rules_vec[i].sensor2->value > rules_vec[i].ref2 && strstr(rules_vec[i].op, "AND") != NULL)
            {
                *rules_vec[i].out = 1;
            }

            else if ((!strcmp(&rules_vec[i].operation, ">") && rules_vec[i].sensor->value > rules_vec[i].ref) || (!strcmp(&rules_vec[i].operation2, ">") && rules_vec[i].sensor2->value > rules_vec[i].ref2 && strstr(rules_vec[i].op, "OR") != NULL))
            {
                *rules_vec[i].out = 1;
            }
            else if ((!strcmp(&rules_vec[i].operation, ">") && rules_vec[i].sensor->value > rules_vec[i].ref) || (!strcmp(&rules_vec[i].operation2, "<") && rules_vec[i].sensor2->value < rules_vec[i].ref2 && strstr(rules_vec[i].op, "OR") != NULL))
            {
                *rules_vec[i].out = 1;
            }
            else if ((!strcmp(&rules_vec[i].operation, "<") && rules_vec[i].sensor->value < rules_vec[i].ref) || (!strcmp(&rules_vec[i].operation2, "<") && rules_vec[i].sensor2->value < rules_vec[i].ref2 && strstr(rules_vec[i].op, "OR") != NULL))
            {
                *rules_vec[i].out = 1;
            }
            else if ((!strcmp(&rules_vec[i].operation, "<") && rules_vec[i].sensor->value < rules_vec[i].ref) || (!strcmp(&rules_vec[i].operation2, ">") && rules_vec[i].sensor2->value > rules_vec[i].ref2 && strstr(rules_vec[i].op, "OR") != NULL))
            {
                *rules_vec[i].out = 1;
            }
        }
    }
}

void measure_power(float **vec_sec, float **vec_hour, int moteID, float volt, float light, float curr, float temp, float humi)
{

    time(&atual);
    struct tm *info;
    info = localtime(&atual);
    printf("Current local time and date: %s", asctime(info));
    double teste = difftime(atual, old);
    //printf("Diff seconds%lf:\n", teste);

    if (teste >= 1)
    {
        old = atual;
        int ac = 0, lights = 0, humidity = 0;
        float total_power;

        if (moteID == 1)
        {
            for (int i = 0; i < N_ACTUATORS; i++)
            {
                if (strstr(outputs_vetor[i].name, "HEAT") != NULL)
                {
                    //Assume that AC consumption is constant =8A
                    if (outputs_vetor[i].on == 1 || outputs_vetor[i].off == 1)
                    {
                        ac += 8;
                    }
                }
                else if (strstr(outputs_vetor[i].name, "LIGHT") != NULL)
                {
                    //Assume that Light consumption is constant =4A
                    if (outputs_vetor[i].on == 1)
                    {
                        lights += 4;
                    }
                }
                else if (strstr(outputs_vetor[i].name, "HUMI") != NULL)
                {
                    //Assume that humidifier consumption is constant =8A
                    if (outputs_vetor[i].on == 1 || outputs_vetor[i].off == 1)
                    {
                        humidity += 8;
                    }
                }
                // For Robots (current sensor) and for carpets (voltage) assume constant pwer consumption
                // Carpets 10A
                // Robots 10A

                if (volt >= 0)
                {
                    total_power = 230 * (ac + lights + humidity + 10 + 10);
                }
                else
                {
                    total_power = 230 * (ac + lights + humidity + 10);
                }
            }
        }
        else if (moteID == 2)
        {
            for (int i = 0; i < N_ACTUATORS; i++)
            {
                if (strstr(outputs_vetor[i].name, "FORNO") != NULL)
                {
                    //Assume that FORNO consumption rise 80mA for with degree of temp
                    if (outputs_vetor[i].on == 1)
                    {
                        ac = temp * 0.080;
                    }
                }
                else if (strstr(outputs_vetor[i].name, "LIGHT") != NULL)
                {
                    //Assume that Light consumption is constant =4A
                    if (outputs_vetor[i].on == 1)
                    {
                        lights += 4;
                    }
                }
                else if (strstr(outputs_vetor[i].name, "HUMI") != NULL)
                {
                    //Assume that humidifier consumption is constant =8A
                    if (outputs_vetor[i].on == 1 || outputs_vetor[i].on == 1)
                    {
                        humidity += 8;
                    }
                }
                // For Robots (current sensor) and for carpets (voltage) assume constant pwer consumption
                // Carpets 10A
                // Robots 10A

                if (volt >= 0)
                {
                    total_power = 230 * (ac + lights + humidity + curr + 10);
                }
                else
                {
                    total_power = 230 * (ac + lights + humidity + curr);
                }
            }
        }
        //printf("Total Power:%.1f W\n", total_power);
        if (count_hour < 60)
        {
            if (count_sec < 60)
            {
                vec_sec[moteID][count_sec] = total_power / 1000;
                printf("Instant power: %.2f kW\n", vec_sec[moteID][count_sec]);
                count_sec++;
            }
            else
            {
                count_sec = 0;
                double sum = 0;
                for (int j = 0; j < 60; j++)
                {
                    sum = sum + vec_sec[moteID][j];
                }

                vec_hour[moteID][count_hour] = sum / 60;
                printf(" 60sec medium power: %.2f kW\n", vec_hour[moteID][count_hour]);

                count_hour++;
            }
        }
    }
    else
    {
        return;
    }
}

void establish_DB_connection(PGconn *conn , PGresult *res ,const char *dbconn)
{
    
    dbconn = "host = 'db.fe.up.pt' dbname = 'sinf2021a35' user = 'sinf2021a35' password = 'ZbnBodLV'";
    //EXAMPLE : dbconn = "host = 'db.fe.up.pt' dbname = 'sinf1920e32' user = 'sinf1920e32' password = 'QWTTIjZl'";

    conn = PQconnectdb(dbconn);
    //PQexec(conn, "SET search_path TO gman_a35");

    if (!conn)
    {
        printf( "libpq error: PQconnectdb returned NULL. \n\n");
        PQfinish(conn);
        exit(1);
    }

    else if (PQstatus(conn) != CONNECTION_OK)
    {
        printf( "Connection to DB failed: %s", PQerrorMessage(conn));
        PQfinish(conn);
        exit(1);
    }

    else
    {
        printf("Connection OK \n");
        //res = PQexec(conn, "INSERT INTO test_1920 (id, name, age) VALUES (1, 'Jane Doe', 32)");
        PQfinish(conn);
    }
    return;
}


/*
void insert_values (PGconn *conn, char *table_name, char *column_names , char *values)
{

    char columns[10][10];
    ////////////////
    
    char exe_columns = "SELECT COLUMN_NAME 
                FROM INFORMATION_SCHEMA.COLUMNS
                WHERE TABLE_NAME = "
    strcat(exe_columns,table_name);

    PQexec(conn, exe_columns);
    
    /////////////////
    char *token;
    token = strtok (column_names, "|");
    int i = 0;
    while(token != NULL)
    {
        printf("%s\n",token);
        strcpy(columns[i], token);
        token = strtok (NULL, "|");
        i++;
    }



    char execution [100] = "INSERT INTO ";
    strcat(execution, table_name);
    strcat(execution, " (");

    i = 0;
    while(columns[i]!= NULL){

        strcat(execution, columns[i]);

        if (columns[i+1]!= NULL){
            strcat(execution, ",");
        }   
        i++;
    }
    strcat(execution, ") ");
    strcat(execution, "VALUES (");



    token = strtok (values, "|");
    while(token != NULL)
    {
        printf("%s\n",token);
        strcat(execution, ",");
        token = strtok (NULL, "|");
    }

    strcat(execution, ")");

    //(name, age) VALUES (“Anna”, 22)


    PGresult *res = PQexec(conn, execution);

    return PQresultStatus(res) == PGRES_COMMAND_OK;

}

void delete_values (PGconn *conn, char *table_name, char *PRIMARY_KEY, int id)
{
    //"DELETE FROM employee WHERE id = "

    char execution [100] ="DELETE FROM ";
    strcat(execution, table_name);
    strcat(execution, " WHERE ");
    strcar (execution, PRIMARY_KEY);
    strcat(execution," = ");
    
    strcat(execution, itoa(id));
    

    PGresult *res = PQexec(conn, execution);

    return PQresultStatus(res) == PGRES_COMMAND_OK;
}


void drop_all (PGconn *conn)
{

    //DROP TABLE <table_name> CASCADE

    char execution [100] = "DROP TABLE  ";
    PGresult *res;
    int retorno;


    char table_names [10][15];
    strcpy(table_names [0],"SENSOR_VEC");
    strcpy(table_names [1],"SECTION");
    strcpy(table_names [2],"MOTE");
    strcpy(table_names [3],"SENSOR");
    strcpy(table_names [4],"SUBRULE");
    strcpy(table_names [5],"OP_R_SUBR");
    strcpy(table_names [6],"RULE");
    strcpy(table_names [7],"ACTUATOR");
    strcpy(table_names [8],"ACTUATOR_VEC");
    

    int i = 0;
    while (i<9){

        strcat(execution,table_names[i]);
        strcat(execution," CASCADE");
        res = PQexec(conn,execution);
        if(res != PGRES_COMMAND_OK){
            print("Error Drop Table %d\n",i);
            retorno = 1;
        }

        execution[0] = '\0';

        i++;
    }

    
    if (retorno==1){
        return 1;
    }
    return 0;  
}

void update_values (PGconn *conn, char *table_name, char *PRIMARY_KEY, int id, char *column, float value)
{
    //UPDATE students SET age = 17 WHERE id = 7

    char execution [100] ="UPDATE ";
    strcat(execution, table_name);
    strcat(execution, " SET ");
    strcat(execution, column);
    strcat(execution, " = ");
    strcat(execution, itoa(value));
    strcat(execution, " WHERE ");
    strcat(execution, PRIMARY_KEY);
    strcat(execution, " = ");
    strcat(execution, itoa(id));

    PGresult *res = PQexec(conn, execution);

    return PQresultStatus(res) == PGRES_COMMAND_OK;

    
}

*/
int main()
{
    FILE *f_terminal;

    establish_DB_connection(conn,res,dbconn);
    

    int n_atuadores = load_sensorconfig();
    
    time(&old);
    //check_OK();
    int rules_number = load_rules(n_atuadores);
    return 0;
    int moteID;
    float voltage, light, current, temperature, rel_humidity, humidity_temp;

    int r = 60, i, j, count;

    float *power_sec[r];
    float *power_hour[r];
    int old_actuator_value[MAX_RULES];

    for (i = 0; i < n_atuadores; i++)
    {
        outputs_vetor[i].on = 0;
        outputs_vetor[i].off = 0;
        //printf("%s ON:%d OFF:%d\n", outputs_vetor[i].name, outputs_vetor[i].on, outputs_vetor[i].off);
    }

    for (j = 0; j < MAX_RULES; j++)
        old_actuator_value[j] = 3;

    for (j = 0; j < N_MOTES; j++)
        init[j] = 1;

    for (i = 0; i < r; i++)
    {
        power_sec[i] = (float *)malloc(N_MOTES * sizeof(float));
        power_hour[i] = (float *)malloc(N_MOTES * sizeof(float));
    }

    // Init. seconds_vector
    for (i = 0; i < r; i++)
        for (j = 0; j < N_MOTES; j++)
            power_sec[i][j] = 0;

    //Init. hour_vector
    for (i = 0; i < r; i++)
        for (j = 0; j < N_MOTES; j++)
            power_hour[i][j] = 0;

    while (1)
    {
        f_terminal = fopen("/tmp/ttyV10", "r");

        if (f_terminal == NULL)
        {
            printf("Error terminal ttyV10\n");
            exit(EXIT_FAILURE);
        }

        if (fgets(str, MAX_CHAR, f_terminal) != NULL)
        {
            //printf("Passed\n");
            //printf("VALOR_LIDO: %s", str);
            if (check_message_start() == 1)
            {
                moteID = (int)get_num_dec(15);
                voltage = get_voltage(get_num_dec(30));
                light = get_light(get_num_dec(36));
                current = get_current(get_num_dec(42));
                temperature = get_temperature(get_num_dec(48));
                rel_humidity = get_relative_humidity(get_num_dec(54));
                humidity_temp = get_temp_humidity(rel_humidity, temperature, get_num_dec(54));

                if (moteID == 2)
                {
                    temperature = temperature * 10;
                }
                int j = 0;
                while (j < N_SENSOR_MOTE)
                {

                    if (strstr(motes[moteID - 1].pos[j].name, "VOLT") != NULL)
                    {
                        motes[moteID - 1].pos[j].value = voltage;
                    }
                    else if (strstr(motes[moteID - 1].pos[j].name, "LIGHT") != NULL)
                    {
                        motes[moteID - 1].pos[j].value = light;
                    }
                    else if (strstr(motes[moteID - 1].pos[j].name, "CURR") != NULL)
                    {
                        motes[moteID - 1].pos[j].value = current;
                    }
                    else if (strstr(motes[moteID - 1].pos[j].name, "TEMP") != NULL)
                    {
                        motes[moteID - 1].pos[j].value = temperature;
                    }
                    else if (strstr(motes[moteID - 1].pos[j].name, "HUM") != NULL)
                    {
                        motes[moteID - 1].pos[j].value = humidity_temp;
                    }
                    j++;
                }
            }

            print_mote();
        }
        else
        {
            printf("Error on Message !\n ");
        }
        if (init[moteID - 1] != 1)
        {

            outputs_update(rules_number);
            measure_power(power_sec, power_hour, moteID, voltage, light, current, temperature, humidity_temp);
            check_values(old_actuator_value, n_atuadores, rules_number, moteID);
            new_values(moteID);
            write_2_RGB();
        }
        else
        {
            new_values(moteID);
        }
        printf("\n");
    }

    fclose(f_terminal);
    free(power_hour);
    free(power_sec);
    return 0;
}
