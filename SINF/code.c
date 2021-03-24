#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

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
void load_sensorconfig(void);
void check_OK(void);
int load_rules(void);
void print_mote(void);
void write_2_RGB();
void outputs_update(void);

char str[MAX_CHAR];

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

void check_values (){



    int i = 0;
    while(rules_vec[i].sensor != NULL){
        
        //char op [2];
        //strcpy(op, &rules_vec[i].operation);
        //printf("operation: %s\n",op);
        printf("Rise %s\n",rules_vec[i].sensor->rise);
        //printf("Ref  %d\n",rules_vec[i].ref);
        printf("OUTPUT: %d\n",*rules_vec[i].out);

        if (*rules_vec[i].out==1 && strcmp(rules_vec[i].sensor->rise,"-1")==0){

            strcpy(rules_vec[i].sensor->rise,"1");


        }else if (*rules_vec[i].out == 1 && strcmp(rules_vec[i].sensor->rise,"1")==0 ){
            printf("DEvia ficar negativo\n");
            strcpy(rules_vec[i].sensor->rise,"-1");
            printf("Mas Rise: %s\n",rules_vec[i].sensor->rise);
        }else if (strcmp(rules_vec[i].sensor->rise, "") == 0){
            printf("Always Here\n");
            strcpy(rules_vec[i].sensor->rise,"1");
        }

        i++;
    }
}

void new_values(int moteID)
{

    //change variables in file
    FILE *f_msgcreator;
    FILE *f_sensorconfig;
    char frases [N_MOTES][300];
    char frase [300];
    char *token;
    int numero_motes=0;
    int n_sensores=0;
    
    

   
    if (moteID == 1){

        f_msgcreator = fopen("MsgCreatorConf.txt","r");

    }else if (moteID == 2){

        f_msgcreator = fopen("msg2/MsgCreatorConf.txt","r");
    }

    if (f_msgcreator == NULL){

        printf("Error new_values 2\n");
        exit(EXIT_FAILURE);
    }




    fgets(frase,300,f_msgcreator);
    

    token = strtok(frase,"[");
    token = strtok(NULL,"[");
    char aux[10];
    strcpy(aux, token);

    //printf("token i: %s\n",token);
    n_sensores=0;
    token = (strtok(token,","));
    
    if ( token != NULL){

        //mais do que 1 sensor
        while (token != NULL){

            n_sensores++;
            //printf("token: %s\n",token);
            token =(strtok(NULL,","));
        }
    }else{
        
        n_sensores=1;
    }
    
    
    //printf("%d\n",n_sensores);
    char sensores [n_sensores][2];

    int i = 0;
    while(i < n_sensores){

        if (n_sensores==1){
            token = (strtok(aux,"]"));
            strcpy(sensores[i],token);
            break;
        }
        
        //printf("%s\n",aux);
        if(i==0){
            token = (strtok(aux,","));
        }else if(i==n_sensores-1){
            token = strtok(NULL,"]");

        }else{
            token = (strtok(NULL,","));
        }
        strcpy(sensores[i],token);
        //printf("sensor: %s\n",sensores[i]);
        i++;
    }

    //////////////////////////////////////ordem dos sensores acima

    i = 0;
    char final_string [300];
    
    fseek(f_msgcreator,0,SEEK_SET);
    fgets(frase,300,f_msgcreator);
    char frase2 [300];
    fseek(f_msgcreator,0,SEEK_SET);
    fgets(frase2,300, f_msgcreator);

    char *identifier1;
    char *identifier2;
    
    final_string[0]='\0';
    //printf("inicio: %s\n",final_string);
    token=strtok_r(frase,"[",&identifier1);
    //printf("token 1: %s\n",token);
    strcat(final_string,token);
    //printf("final: %s\n",final_string);

    token=strtok_r(NULL,"[", &identifier1);
    //printf("token 2: %s\n",token);
    strcat(final_string,"[");
    strcat(final_string,token);
    strcat(final_string, "[");
    //printf("final2: %s\n",final_string);
    int posi;


    printf("Inicio1\n");
    while (i < n_sensores){

        token=strtok_r(NULL,"[", &identifier1);
        printf("token while: %s\n",token);

        if (strcmp(sensores[i],"1")==0){
            
            int ii=0;
            printf("Light\n");
            while (motes[moteID].pos[ii].name != NULL){
                
                printf("Light While\n");
                printf("%d\n",ii);
                //printf("mote: %s\n",motes[moteID-1].pos[ii].name);
                if (strstr(motes[moteID-1].pos[ii].name, "LIGHT") != NULL){
                    //printf("id: %d\n",moteID-1);
                    //printf("mote: %s\n",motes[moteID-1].pos[ii].name);

                    posi=ii;
                    printf("%s\n",motes[moteID-1].pos[ii].name);
                    break;
                }
                ii++;
                //printf("ii2: %d\n",ii);
                //printf("mote2: %s\n",motes[moteID-1].pos[ii].name);
            }
            //printf("%d\n",posi);
            token = strtok_r(token,",",&identifier2);
            strcat(final_string,"[");
            strcat(final_string,token);
            strcat(final_string,",");
            token = strtok_r(NULL,",",&identifier2);
            strcat(final_string,token);
            strcat(final_string,",");
            token = strtok_r(NULL,",",&identifier2);
            strcat(final_string,token);
            strcat(final_string,",");
            //printf("In New Values\n");
            printf("Mote Name %s\n",motes[moteID-1].pos[posi].name);
            printf("MOte RIse %s\n",motes[moteID-1].pos[posi].rise);
            strcat(final_string,motes[moteID-1].pos[posi].rise);
            if(i==n_sensores-1){
                strcat(final_string,"]]");
            }else{
                strcat(final_string,"],");
            }
            printf("L-> %s\n",final_string);

        }else if (strcmp(sensores[i],"3")==0){

            int ii=0;
            
            //strncmp(motes[moteID].pos[ii].name,"\0",3)
            while (motes[moteID].pos[ii].name != NULL){

                if (strstr(motes[moteID-1].pos[ii].name, "TEMP") != NULL){
                    //printf("id: %d\n",moteID-1);
                    //printf("mote: %s\n",motes[moteID-1].pos[ii].name);

                    posi=ii;
                    printf("%s\n",motes[moteID-1].pos[ii].name);
                    break;
                }
                ii++;
            }
            //printf("%d\n",posi);
            token = strtok_r(token,",",&identifier2);
            strcat(final_string,"[");
            strcat(final_string,token);
            strcat(final_string,",");
            token = strtok_r(NULL,",",&identifier2);
            strcat(final_string,token);
            strcat(final_string,",");
            token = strtok_r(NULL,",",&identifier2);
            strcat(final_string,token);
            strcat(final_string,",");
            //printf("In New Values\n");
            //printf("Mote Name %s\n",motes[moteID-1].pos[posi].name);
            //printf("MOte RIse %s\n",motes[moteID-1].pos[posi].rise);
            strcat(final_string,motes[moteID-1].pos[posi].rise);
             if(i==n_sensores-1){
                strcat(final_string,"]] ");
            }else{
                strcat(final_string,"],");
            }
            //printf("in function new\n");
            //printf("id: %d\n",moteID);
            //printf("grandeza: %s\n",motes[moteID-1].pos[posi].name);
            //printf("rise temp:%s\n",motes[moteID-1].pos[posi].rise);
            //printf("%s\n",final_string);


        }else if (strcmp(sensores[i],"4")==0){
            int ii=0;
            

            while (motes[moteID].pos[ii].name != NULL){

                if (strstr(motes[moteID-1].pos[ii].name, "HU") != NULL){
                    //printf("id: %d\n",moteID-1);
                    //printf("mote: %s\n",motes[moteID-1].pos[ii].name);

                    posi=ii;
                    printf("%s\n",motes[moteID-1].pos[ii].name);
                    break;
                }
                ii++;
            }
            //printf("%d\n",posi);
            token = strtok_r(token,",",&identifier2);
            strcat(final_string,"[");
            strcat(final_string,token);
            strcat(final_string,",");
            token = strtok_r(NULL,",",&identifier2);
            strcat(final_string,token);
            strcat(final_string,",");
            token = strtok_r(NULL,",",&identifier2);
            strcat(final_string,token);
            strcat(final_string,",");
            //printf("2token %s\n",token);
            //printf("In New Values\n");
            //printf("Mote Name %s\n",motes[moteID-1].pos[posi].name);
            //printf("MOte RIse %s\n",motes[moteID-1].pos[posi].rise);
            strcat(final_string,motes[moteID-1].pos[posi].rise);
            if(i==n_sensores-1){
                strcat(final_string,"]]");
            }else{
                strcat(final_string,"],");
            }
            //printf("%s\n",final_string);
        }
        else{
            printf("NOT\n");
            strcat(final_string,"[");
            strcat(final_string,token);
        }
        i++;
    }


    //printf("finaly %s\n",final_string);
    fclose(f_msgcreator);

    

    if (moteID == 1){

        f_msgcreator = fopen("MsgCreatorConf.txt","w");
        strcat(final_string," -i 1");
    }else if (moteID == 2){

        f_msgcreator = fopen("msg2/MsgCreatorConf.txt","w");
        strcat(final_string," -i 2");
    }

    if (f_msgcreator == NULL){

        printf("Error new_values 3\n");
        exit(EXIT_FAILURE);
    }
    


    
    printf("finaly2 %s\n",final_string);
    //fseek(f_msgcreator,0,SEEK_SET);
    fputs(final_string,f_msgcreator);
    //fputs(final_string,f_msgcreator);

    

    final_string[0]='\0';
    fclose(f_msgcreator);

}

void load_sensorconfig(void)
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

            char *newline = strrchr(line, '\n');
            if (newline != NULL)
                *newline = '\0';

            //printf("%s\n", line);
            token = strtok(line, ":");
            sprintf(dec_to_str, "%d", i + 1);

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
                else
                {
                    i++;
                    sprintf(dec_to_str, "%d", i + 1);
                }
            }
            // write inputs on mote[]
            token = strtok(inputs, ",");
            while (token != NULL)
            {
                strcpy(motes[i].pos[j].name, token);
                j++;
                token = strtok(NULL, ",");
            }
            j = 0;

            // write outputs on outputs_vector[]
            token = strtok(outputs, ",");

            while (token != NULL && cnt < N_ACTUATORS)
            {

                strcpy(outputs_vetor[cnt].name, token);
                cnt++;
                token = strtok(NULL, ",");
            }
        }
        else
        {
            //check_OK();
            break;
        }
    }

    fclose(f);
    return;
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
int load_rules(void)
{

    FILE *f;
    char line[2 * MAX_CHAR];
    int i = 0, k = 0, j = 0;
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
        char *newline = strrchr(line, '\n');
        if (newline != NULL)
            *newline = '\0';

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
        sprintf(dec_to_str, "%d", i + 1);
        while (i < N_MOTES)
        {
            if (strstr(token, dec_to_str) != NULL)
            {

                token = strtok(NULL, " ");

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

                    break;
                }
                else
                {

                    token = strtok(NULL, " ");
                    token = strtok(NULL, " ");

                    if (strcpy(subject2, token) == NULL)
                    {

                        printf("ERROR on load_rules subject2\n");
                        fclose(f);
                        return -1;
                    }

                    token = strtok(NULL, "\0");

                    if (strcpy(predicate, token) == NULL)
                    {

                        printf("ERROR on load_rules predicate(2)\n");
                        fclose(f);
                        return -1;
                    }

                    //puts(subject);printf("AND?%d\n",is_and);puts(subject2);puts(predicate);
                    break;
                }
            }
            else
            {

                i++;
                sprintf(dec_to_str, "%d", i + 1);
            }
        }
        //printf("Sucess Mote:%d\n",i+1);
        int true_i=i;
        i=0;

        while (1)
        {
            // Load 1st part --> Sensor;Oper;Ref
            //puts(subject);
            //puts(predicate);
            //printf("%s\n",motes[i].pos[j].name);

            if (strstr(subject, motes[i].pos[j].name) != NULL)
            {
                rules_vec[k].sensor = &motes[i].pos[j];
                rules_vec[k].operation = subject[strlen(motes[i].pos[j].name)];
                //printf("%c\n", rules_vec[k].operation);
                sscanf(&subject[strlen(motes[i].pos[j].name) + 1], "%d", &rules_vec[k].ref);

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
                    rules_vec[k].is_complex = 0;
                    break;
                }
            }
            else
            {
                if (j < N_SENSOR_MOTE)
                {
                    j++;
                    if (j == N_SENSOR_MOTE)
                    {
                        j = 0;
                        i++;
                    }
                }
                else
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
                sscanf(&subject2[strlen(motes[i].pos[j].name) + 1], "%d", &rules_vec[k].ref2);
                break;
            }
            else
            {
                if (j < N_SENSOR_MOTE)
                {
                    j++;
                    if (j == N_SENSOR_MOTE)
                    {
                        j = 0;
                        i++;
                    }
                }
                else
                {
                    printf("Error on detect sensor in mote %d\n", i + 1);
                    fclose(f);
                    return -1;
                }
            }
        }
        i = true_i;
        j = 0;
        //Load 2nd part (predicate) --> Atuactor; Cond:ON/OFF

        while (j < N_ACTUATORS)
        {

            if (strstr(predicate, outputs_vetor[j].name) != NULL)
            {

                char *token = strtok(predicate, ":");
                token = strtok(NULL, ":");
                // If ON, then we know that when the rules is verified(rgb_matrix_write=1), we need to
                // turn ON the actuator

                if (strstr(token, "ON") != NULL)
                {
                    rules_vec[k].out = &outputs_vetor[j].on;
                    k++;
                    break;
                }
                else
                {
                    // If OFF, then we know that when the rules is verified(rgb_matrix_write=1), we need to
                    // turn OFF the actuator
                    if (strstr(token, "OFF") != NULL)
                    {
                        rules_vec[k].out = &outputs_vetor[j].off;
                        k++;
                        break;
                    }
                    else
                    {
                        printf("ERROR on detect atuactor!\n");
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

        i = 0;
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

    int n_seccões = N_MOTES;

    int celulas_matriz = n_atuadores + n_seccões - 1;

    FILE *f;
    f = fopen("RGBMatrixConf.txt", "w");

    FILE *fp;
    fp = fopen("matrix.txt", "w");

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

    for (int i = 0; i < N_ACTUATORS; i++)
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
                    printf("Skipped");
                }
            }
            seccao_atual++;
            sprintf(str, "%d", seccao_atual);
        }
    }

    if (par == 0)
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

    if (par == 1)
    {
        fprintf(fp, "[");

        for (int i = 0; i < celulas_matriz; i++)
        {

            for (int j = 0; j < ((celulas_matriz - celulas_matriz / 2) / 2); j++)
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
    free(file_contents);
}

void outputs_update(void)
{

    for (int i = 0; i < MAX_RULES; i++)
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
            //if (strstr(rules_vec[i].op, "AND")!=NULL)
            //printf("%c", rules_vec[i].operation);
            if (!strcmp(&rules_vec[i].operation, ">") && rules_vec[i].sensor->value > rules_vec[i].ref && !strcmp(&rules_vec[i].operation2, ">") && rules_vec[i].sensor2->value > rules_vec[i].ref2 && strstr(rules_vec[i].op, "AND")!=NULL)
            {
                *rules_vec[i].out = 1;
            }
            else if (!strcmp(&rules_vec[i].operation, ">") && rules_vec[i].sensor->value > rules_vec[i].ref && !strcmp(&rules_vec[i].operation2, "<") && rules_vec[i].sensor2->value < rules_vec[i].ref2 && strstr(rules_vec[i].op, "AND")!=NULL)
            {
                *rules_vec[i].out = 1;
            }
            else if (!strcmp(&rules_vec[i].operation, "<") && rules_vec[i].sensor->value < rules_vec[i].ref && !strcmp(&rules_vec[i].operation2, "<") && rules_vec[i].sensor2->value < rules_vec[i].ref2 && strstr(rules_vec[i].op, "AND")!=NULL)
            {
                *rules_vec[i].out = 1;
            }
            else if (!strcmp(&rules_vec[i].operation, "<") && rules_vec[i].sensor->value < rules_vec[i].ref && !strcmp(&rules_vec[i].operation2, ">") && rules_vec[i].sensor2->value > rules_vec[i].ref2 && strstr(rules_vec[i].op, "AND")!=NULL)
            {
                *rules_vec[i].out = 1;
            }

            else if ((!strcmp(&rules_vec[i].operation, ">") && rules_vec[i].sensor->value > rules_vec[i].ref) || (!strcmp(&rules_vec[i].operation2, ">") && rules_vec[i].sensor2->value > rules_vec[i].ref2 && !strcmp(rules_vec[i].op, "OR")))
            {
                *rules_vec[i].out = 1;
            }
            else if ((!strcmp(&rules_vec[i].operation, ">") && rules_vec[i].sensor->value > rules_vec[i].ref) || (!strcmp(&rules_vec[i].operation2, "<") && rules_vec[i].sensor2->value < rules_vec[i].ref2 && !strcmp(rules_vec[i].op, "OR")))
            {
                *rules_vec[i].out = 1;
            }
            else if ((!strcmp(&rules_vec[i].operation, "<") && rules_vec[i].sensor->value < rules_vec[i].ref) || (!strcmp(&rules_vec[i].operation2, "<") && rules_vec[i].sensor2->value < rules_vec[i].ref2 && !strcmp(rules_vec[i].op, "OR")))
            {
                *rules_vec[i].out = 1;
            }
            else if ((!strcmp(&rules_vec[i].operation, "<") && rules_vec[i].sensor->value < rules_vec[i].ref) || (!strcmp(&rules_vec[i].operation2, ">") && rules_vec[i].sensor2->value > rules_vec[i].ref2 && !strcmp(rules_vec[i].op, "OR")))
            {
                *rules_vec[i].out = 1;
            }
        }
    }
}

int main()
{
    FILE *f_terminal;

    load_sensorconfig();

    int rules_number = load_rules();
    int rise_temp = 1, rise_light = 1, rise_hum = 1;
    int wrong_values = 0;
    int moteID;
    float voltage, light, current, temperature, rel_humidity, humidity_temp;

    while (1)
    {
        f_terminal = fopen("/tmp/ttyV10", "r");

        if (f_terminal == NULL)
        {
            printf("ERRO\n");
            exit(EXIT_FAILURE);
        }

        if (fgets(str, MAX_CHAR, f_terminal) != NULL)
        {

            char *newline = strrchr(str, '\n');
            if (newline != NULL)
                *newline = '\0';
            //printf("Passed\n");
            printf("VALOR_LIDO: %s\n", str);
            if (check_message_start() == 1)
            {
                moteID = (int)get_num_dec(15);
                voltage = get_voltage(get_num_dec(30));
                light = get_light(get_num_dec(36));
                current = get_current(get_num_dec(42));
                temperature = get_temperature(get_num_dec(48));
                rel_humidity = get_relative_humidity(get_num_dec(54));
                humidity_temp = get_temp_humidity(rel_humidity, temperature, get_num_dec(54));
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
        
        printf("CHECK VALUES\n");
        //new_values(moteID);

        outputs_update();
        //check_values();
        //new_values(moteID);
        write_2_RGB();
    }
    fclose(f_terminal);
}
