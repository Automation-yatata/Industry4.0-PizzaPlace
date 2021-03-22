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

void check_values(int temp, int *rise_temp, int light, int *rise_light, int hum, int *rise_hum, int *wrong)
{

    //function for changing variation of sensor's functions
    //wrong variable changes when some condition is met so changes can be applied to file

    if (temp > 26)
    {

        *rise_temp = -1;
        *wrong = 1;
    }
    else if (temp < 24)
    {

        *rise_temp = 1;
        *wrong = 1;
    }

    if (hum > 60)
    {

        *rise_hum = -1;
        *wrong = 1;
    }
    else if (hum < 40)
    {

        *rise_hum = 1;
        *wrong = 1;
    }

    if ((temp < 26 && temp > 24) && (hum < 60 && hum > 40))
    {

        *wrong = 0;
    }
}

void new_values(FILE *f, int *rise_temp, int *rise_light, int *rise_hum)
{

    //change variables in file

    fseek(f, 0, SEEK_SET);
    //fputc(teste,f);
    //fprintf(f, "-n 1 -l 100 -f 1 -c 1 -s [0,1,2,3,4] -d [['U',0.0,5.0,2.0],['L',400.0,700.0,1],['U',2.0,10.0,2.0],['L',20.0,30.0,%d],['L',30.0,70.0,%d]] -i 1",*rise_temp, *rise_hum);
    fprintf(f, "-n 1 -l 100 -f 1 -c 1 -s [0,1,2,3,4] -d [['P',10,50,0.1],['L',400.0,700.0,1],['P',10,67,0.15],['L',20.0,30.0,%d],['L',30.0,70.0,%d]] -i 1", *rise_temp, *rise_hum);
    //fseek(f,0,SEEK_CUR);
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
    char predicate[MAX_CHAR];

    while (fgets(line, 2 * MAX_CHAR, f) != NULL && k < MAX_RULES)
    {
        //printf("%s\n", line);
        j = 0;

        token = strtok(line, ":");
        //puts(token);

        // i related to N_MOTES
        // j related to pos[] in layout struct (whats sensor; 0(volt) etc)
        // k  related to position in RULES vector

        while (i < N_MOTES)
        {

            sprintf(dec_to_str, "%d", i + 1);
            if (strstr(token, dec_to_str) != NULL)
            {
                token = strtok(NULL, " ");

                if (strcpy(subject, token) == NULL)
                {
                    printf("ERROR\n");
                    fclose(f);
                    return -1;
                }
                token = strtok(NULL, "\n");

                if (strcpy(predicate, token) == NULL)
                {
                    printf("ERROR\n");
                    fclose(f);
                    return -1;
                }
                //puts(subject);puts(predicate);
                break;
            }
            else
            {
                i++;
                sprintf(dec_to_str, "%d", i + 1);
            }
        }
        while (1)
        {
            // Load 1st part --> Sensor;Oper;Ref
            //puts(subject);puts(motes[i].pos[j].name);
            //printf("%d\n",j);

            if (strstr(subject, motes[i].pos[j].name) != NULL)
            {
                rules_vec[k].sensor = &motes[i].pos[j];

                // For operation one option is put ascii number and when check the condition compare to ascii
                rules_vec[k].operation = subject[strlen(motes[i].pos[j].name)];
                //printf("%c\n", rules_vec[i].operation);
                sscanf(&subject[strlen(motes[i].pos[j].name) + 1], "%d", &rules_vec[k].ref);
                break;
            }
            else
            {
                if (j < N_SENSOR_MOTE)
                {
                    j++;
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

    /*while (i < k)
    {
        printf("%s_%c_%d || Actuator ON|OFF %d\n", rules_vec[i].sensor->name,
               rules_vec[i].operation, rules_vec[i].ref, *rules_vec[i].out);
        i++;
    }*/
    fclose(f);
    return k;
}
void print_mote(void)
{

    for (int i = 0; i < N_MOTES; i++)
    {
        int j=0;
        printf("[%d] %s:%.2f %s:%.2f %s:%.2f %s:%.2f %s:%.2f\n", i+1, motes[i].pos[j].name,
        motes[i].pos[j].value,motes[i].pos[j+1].name, motes[i].pos[j+1].value,motes[i].pos[j+2].name, motes[i].pos[j+2].value,motes[i].pos[j+3].name, motes[i].pos[j+3].value,motes[i].pos[j+4].name, motes[i].pos[j+4].value);
    }
}

void write_2_RGB()
{

    const char *channelRGB = "/tmp/ttyV13";
    FILE *matrix_channel;

    matrix_channel= fopen(channelRGB, "w");

    int n_atuadores=0;
    for (int i=0; i<N_ACTUATORS; i++){
        if (!strncmp(outputs_vetor[i].name,"\0",3)) break;
        else n_atuadores++;
    }

    int n_seccões= N_MOTES;

    int celulas_matriz= n_atuadores + n_seccões-1;

    FILE *f;
    f= fopen("RGBMatrixConf.txt", "w");

    FILE *fp;
    fp= fopen("matrix.txt", "w");

    fprintf(f,"-a %i -b 50", celulas_matriz); 
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

if(celulas_matriz % 2 == 0)
        par=1;
    else
        par=0;


if (par==0){
    fprintf(fp, "[");

    for (int i=0; i<celulas_matriz; i++){


        for (int j=0; j<((celulas_matriz-celulas_matriz/2)/2)+1; j++){
            //printf("j=%i",j);
        fprintf(fp, "%s,", BLACK);
        }

        for(int k=0; k<2; k++){
            if(outputs_vetor[i].on==1)
        fprintf(fp, "%s,", GREEN);
        else if (outputs_vetor[i].off==1)
        fprintf(fp,"%s,", RED);
        else fprintf(fp, "%s,", WHITE);            
        }

        for(int l=0; l<((celulas_matriz-celulas_matriz/2)/2); l++){
            if (l==((celulas_matriz-celulas_matriz/2)/2)-1 && i==celulas_matriz-1)
        fprintf(fp, "%s", BLACK);
        else fprintf(fp, "%s,", BLACK);
        }

    }
    fprintf(fp, "]\n");
}

fclose(fp);

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
printf("%s", file_contents);
/*char ch;

ch = fgetc(fp);
    while (ch != EOF)
    {
        
        fputc(ch, matrix_channel);

        
        ch = fgetc(fp);
    }
    */

    for (int i=0; i<N_ACTUATORS; i++){

        outputs_vetor[i].on=0;
        outputs_vetor[i].off=0;
    }

    //free(ATUATOR);
    fclose(matrix_channel);
    fclose(f);
    
}

void outputs_update(void){

    for (int i=0; i<MAX_RULES; i++){

        if (rules_vec[i].operation==0){
            break;
        }
        else{

            if (!strcmp(&rules_vec[i].operation, ">") && rules_vec[i].sensor->value > rules_vec[i].ref){
                *rules_vec[i].out=1;
            }

            if (!strcmp(&rules_vec[i].operation, "<") && rules_vec[i].sensor->value < rules_vec[i].ref){
                *rules_vec[i].out=1;
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

    //outputs_update();


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
            printf("Passed\n");
            printf("VALOR_LIDO: %s", str);
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

                    if (strstr(motes[moteID-1].pos[j].name, "VOLT") != NULL)
                    {
                        motes[moteID-1].pos[j].value = voltage;
                    }
                    else if (strstr(motes[moteID-1].pos[j].name, "LIGHT") != NULL)
                    {
                        motes[moteID-1].pos[j].value = light;
                    }
                    else if (strstr(motes[moteID-1].pos[j].name, "CURR") != NULL)
                    {
                        motes[moteID-1].pos[j].value = current;
                    }
                    else if (strstr(motes[moteID-1].pos[j].name, "TEMP") != NULL)
                    {
                        motes[moteID-1].pos[j].value = temperature;
                    }
                    else if (strstr(motes[moteID-1].pos[j].name, "HUM") != NULL)
                    {
                        motes[moteID-1].pos[j].value = humidity_temp;
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

        /*check_values(temperature, &rise_temp, light, &rise_light, rel_humidity, &rise_hum, &wrong_values);

        if (wrong_values == 1)
        {
         
            new_values(f_msgcreator, &rise_temp, &rise_light, &rise_hum);
            
        }*/
        outputs_update();
        write_2_RGB();
    }
        fclose(f_terminal);
    
}
