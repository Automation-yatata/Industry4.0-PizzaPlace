#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_CHAR 75

long get_num_dec(int pos);
int check_message_start(void);
float get_voltage(long dec);
float get_light(long dec);
float get_current(long dec);
float get_temperature(long dec);
float get_relative_humidity(long dec);
float get_temp_humidity(float rel_hum, float temp, long dec);

char str[MAX_CHAR];

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

    return (0.625 * 106 * (((float)dec / 4096) * 1.5 / 10) * 1000);
}
float get_current(long dec)
{

    return (0.769 * 105 * (((float)dec / 4096) * 1.5 / 100) * 1000);
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




void check_values(int temp, int* rise_temp, int light, int *rise_light, int hum, int *rise_hum, int *wrong){



    //function for changing variation of sensor's functions
    //wrong variable changes when some condition is met so changes can be applied to file

    if(temp > 26){

        *rise_temp=-1;
        *wrong=1;
        
    }else if (temp < 24){

        *rise_temp=1;
        *wrong=1;

    }    


    if(hum > 60){

        *rise_hum=-1;
        *wrong=1;
        
    }else if (hum < 40){

        *rise_hum=1;
        *wrong=1;

    }    
    


    if((temp<26 && temp>24) && (hum < 60 && hum > 40)){

        *wrong=0;
    }


}




void new_values (FILE *f, int*rise_temp, int* rise_light, int* rise_hum){


    //change variables in file

    fseek(f,0, SEEK_SET);
    //fputc(teste,f);
    fprintf(f, "-n 1 -l 100 -f 1 -c 1 -s [0,1,2,3,4] -d [['U',0.0,5.0,2.0],['L',400.0,700.0,1],['U',2.0,10.0,2.0],['L',20.0,30.0,%d],['L',30.0,70.0,%d]] -i 1",*rise_temp, *rise_hum);
    //fseek(f,0,SEEK_CUR);
    
}

int main()
{

    FILE *f;
    f = fopen ("MsgCreatorConf.txt","r+");

    if (f==NULL){
        printf("ERRROOOO\n");
        exit(EXIT_FAILURE);
    }

    int rise_temp=1, rise_light=1, rise_hum=1;
    int wrong_values = 0;
    

    float moteID, voltage, light, current, temperature, rel_humidity, humidity_temp;

    while (1)
    {

        if (fgets(str, MAX_CHAR, stdin) != NULL)
        {
            //printf("VALOR_LIDO: %s", str);
            if (check_message_start() == 1)
            {
                moteID = get_num_dec(15);
                voltage = get_voltage(get_num_dec(30));
                light = get_light(get_num_dec(36));
                current = get_current(get_num_dec(42));
                temperature = get_temperature(get_num_dec(48));
                rel_humidity = get_relative_humidity(get_num_dec(54));
                humidity_temp = get_temp_humidity(rel_humidity, temperature, get_num_dec(54));

                printf("Mote:%.0f Volt:%.2f Light:%.2f Current:%.2f Temp:%.2f Rel_Hum:%.2f Temp_Hum:%.2f\n", moteID, voltage, light, current, temperature, rel_humidity, humidity_temp);
            }
            else
            {
                printf("Error on Message !\n ");
            }





            check_values(temperature, &rise_temp, light, &rise_light, rel_humidity, &rise_hum, &wrong_values);


            if(wrong_values==1){

                new_values(f, &rise_temp, &rise_light, &rise_hum);
            }




        }
    }

    fclose(f);
}