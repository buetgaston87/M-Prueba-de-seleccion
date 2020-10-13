#define _XOPEN_SOURCE /* necessary for strptime */

/* json-c library */
#include <json-c/json.h>

/* necessary includes dor GNU libmicrohttpd library */
#include <sys/types.h>
#ifndef _WIN32
#include <sys/select.h>
#include <sys/socket.h>
#else
#include <winsock2.h>
#endif
#include <microhttpd.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h> /* atof */


#define BUFFER_SIZE 2048
#define FILENAME "Datos.csv"
#define CITYLENGTH 50

struct temperature 
{
    float max;
    float min;
};

struct weatherData 
{
    struct tm date;
    char city[CITYLENGTH];
    struct temperature temperature;
    float precipitation;
    float cloudiness;
};

struct tm tm1;
struct tm tm2;

struct weatherData weather[BUFFER_SIZE];
struct weatherData  *weatherPointer[BUFFER_SIZE];

char p_date[25];
char p_city[25];
char p_unit[25];

int count_weather;
int count_weatherPointer;

void load_csv_file();
int answer_to_connection (void *cls, struct MHD_Connection *connection,
                          const char *url,
                          const char *method, const char *version,
                          const char *upload_data,
                          size_t *upload_data_size, void ** ptr);

void string_init(char** string_ptr);
void string_free(char** string_ptr);
void string_set(char** destination, char* value);

/**
 * Call with the port number as the only argument.
 * Never terminates (other than by signals, such as CTRL-C).
 */
int main (int argc, char **argv)
{

    if (argc != 2)
    {
        printf ("%s PORT\n", argv[0]);
        return 1;
    }
    
    /* read csv file and save data in a structure */
    load_csv_file();

    struct MHD_Daemon *daemon;

    /* start the server daemon which will listen on PORT(argv[1]) for connections */
    daemon = MHD_start_daemon (MHD_USE_INTERNAL_POLLING_THREAD, atoi (argv[1]), NULL, NULL,
                                &answer_to_connection, NULL, MHD_OPTION_END);
    
    if (NULL == daemon) 
        return 1;

    /* the server daemon runs in the background in its own thread, so let it pause the 
        execution flow in our main function by waiting for the enter key to be pressed */
    getchar ();

    /* stop the daemon so it can do its cleanup tasks */
    MHD_stop_daemon (daemon);

    return 0;
}


/**
 * Replaces a word in a text by another given word.
 */
void replace_word(char *s, char *oldW, char *newW)
{
    char buf[BUFFER_SIZE]; 
    char * pch;

    /* pointer to the first occurrence in s of oldW*/
    pch = strstr(s,oldW);
    
    int len_newW = strlen(newW);
    int len_oldW = strlen(oldW);
    
    while (pch!=NULL)
    {
        /* copy all from s to buf since position of the end of the first occurrence of oldW  */
        strncpy(buf, pch+len_oldW, strlen(pch));
        
        /* copy from newW to s since position of the pointer pch */
        strncpy(pch,newW,len_newW);

        /* copy from buf to s since position of the end of the last newW */
        strncpy(pch+len_newW,buf,strlen(buf));
        
        /* pointer to the first occurrence in s of oldW (is the next oldW of the original s)*/
        pch = strstr(pch+len_newW,oldW);     
    }
}


/**
 * Removes spaces from a given string.
 */
void remove_spaces(char *s) 
{ 
    size_t count = 0; 
    size_t i;
    for (i = 0; s[i]; i++) 
        if (s[i] != ' ') 
            /* count is incremented */
            s[count++] = s[i];
    s[count] = '\0'; 
}


/**
 * Calls replaceWord and remove_spaces functions to give format to line.
 */
void give_format_to_line(char *buf)
{
    replace_word(buf, ",", ".");
    remove_spaces(buf);
    replace_word(buf, ";;", ";_;"); 
}

/**
 *  Check date existing 
 */
int is_valid_date(struct tm tmDate)
{    
    /* copy the entered date to the validate date structure */
    struct tm tmValidateDate;
    memcpy( &tmValidateDate, &tmDate, sizeof(struct tm) );

    /* if timeptr references a date before midnight, January 1, 1970, or if the calendar time cannot be 
    represented, mktime returns –1 cast to type time_t. */
    time_t timeCalendar = mktime( &tmValidateDate );
    if( timeCalendar == (time_t) -1 ) 
        return 0;

    return (
            (tmDate.tm_mday == tmValidateDate.tm_mday) &&
            (tmDate.tm_mon == tmValidateDate.tm_mon) &&
            (tmDate.tm_year == tmValidateDate.tm_year)
            );
}


/**
 * Parses the line and save data in weather structure.
 */
int save_data_in_weather_structure(char *buf, struct weatherData *p_weather)
{
    char *tok;
    char delimit[]=";";
    
    /* initialise everything to zeros first, to not have arbitrary information in all its fields 
        before parsing the time string */
    memset( &tm2, 0, sizeof(struct tm) );
    
    /* parse the line into tokens separated by characters in delimit */
    tok = strtok(buf, delimit);
    /* check if data can be converted to tm type */
    if (!strptime(tok, "%Y/%m/%d", &tm2))
        return 0;
    /* check if date exists */
    if(!is_valid_date(tm2))
        return 0;

    /* initialise everything to zeros first, to not have arbitrary information in all its fields 
        before parsing the time string */
    memset(&p_weather->date, 0, sizeof(struct tm));

    /* save the value in weather structure in tm type */
    strptime(tok, "%Y/%m/%d", &p_weather->date);

    /* parses the next token */
    tok = strtok(NULL, delimit);
    /* save the value in weather structure */
    strcpy(p_weather->city, tok);
    
    tok = strtok(NULL, delimit);
    /* check if data can be converted to float */
    if (atof(tok))
        /* save the value in weather structure in float type */
        p_weather->temperature.max = atof(tok);

    tok = strtok(NULL, delimit);
    if (atof(tok))
        p_weather->temperature.min = atof(tok);
    
    tok = strtok(NULL, delimit);
    if (atof(tok))
        p_weather->precipitation = atof(tok);
    
    tok = strtok(NULL, delimit);
    if (atof(tok))
        p_weather->cloudiness = atof(tok);
    
    return 1;
}


/**
 * Opens the csv file in read mode.
 * Reads line by line and calls functions to give format to the line and parses it 
 * to save data in weather structure.
 */
void load_csv_file()
{
    FILE * fp;
    /* open file in read mode */
    fp = fopen(FILENAME, "r");
    
    if (fp == NULL) 
    {
        printf("Error al abrir el archivo \n");
        exit(1);
    }

    char buf[BUFFER_SIZE];

    struct weatherData *p_weather;
    
    /* pointer to first element of weather structure array */
    p_weather = weather;
    
    /* reads the firs line of the file but not use it */
    fgets(buf, sizeof(buf), fp);
    
    /* reads the next line of the file */
    while (fgets(buf, sizeof(buf), fp))
    {
        /* calls the function to give format to the line */
        give_format_to_line(buf);

        /* calls the function to save line data in weather structure */
        if (save_data_in_weather_structure(buf, p_weather))
        {
            /* next element of weather structure array */
            p_weather++;
            
            count_weather++;
        }
    }

    fclose(fp);
}


/**
 * Get from the weather struct array, the data which match with the city, a supirier date and unit selected.
 */
void get_weather_for_city_and_date()
{                
    struct weatherData *p_weather;    
    
    /* pointer to first element of weather structure array */
    p_weather = weather;
    
    /* initialise everything to zeros first, to not have arbitrary information in all its fields 
        before parsing the time string */    
    memset(&tm1, 0, sizeof(struct tm));

    /* save the value in weather structure in tm type */
    strptime(p_date, "%Y/%m/%d", &tm1);
    
    /* set the global variable count_weatherPointer in zero. 
        To restart the count for a new get request */
    count_weatherPointer = 0;

    size_t i;
    for (i = 0; i < count_weather; i++)
    {
        /* compare the city and date of the line of file and the city and date recieves from get paramaters */
        if ((strcmp(p_weather->city, p_city) == 0) && (difftime(mktime(&p_weather->date), mktime(&tm1)) >= 0))
        {
            /* point to the field of array weather */
            weatherPointer[count_weatherPointer] = p_weather;
            
            /* incremente the global variable */
            count_weatherPointer++;
        }

        /* next element of weather structure array */
        p_weather++;
    }
}


/**
 * Check if unit is Fahrenheit, and converts to it.
 */
float convert_unit(float temp, char *unit)
{
    if (!strcmp(unit, "Fahrenheit"))
    {
        temp = temp * 1.8 + 32;
    }
    return temp;
}


/**
* Return a JSON with the city and a list of the temperture.max, temperture.min, 
* precipitation and date for the city and dates selected.
*/
unsigned char * weather_for_city_and_date_JSON()
{    
    char tmp[128];
    char dateT[50];

    struct weatherData *p_weather;

    /* pointer to first element of weather structure array */
    p_weather = weatherPointer[0];

    /* creating a json object */    
    json_object * jobj = json_object_new_object(); 

    /* creating a json string. We use global variable p_city and not p_weather->city 
        because the structure array could arrive empty */
    json_object *jstring = json_object_new_string(p_city);

    /* creating a json array */
    json_object *jarray = json_object_new_array();

    size_t i;
    for (i = 0; i < count_weatherPointer; i++)
    {                   

        /* creating a json double */
        double d1 = convert_unit(p_weather->temperature.max, p_unit);

        /* json-c library has a problem with precision when double object are converted to string, 
            so use json_object_new_double_s() with the string representation of the double(tmp) to solve it */
        snprintf(tmp, 128, "%0.3f", d1);
        json_object *jdouble1 = json_object_new_double_s(d1, tmp);

        double d2 = convert_unit(p_weather->temperature.min, p_unit);
        snprintf(tmp, 128, "%0.3f", d2);
        json_object *jdouble2 = json_object_new_double_s(d2, tmp);

        double d3 = p_weather->precipitation;
        snprintf(tmp, 128, "%0.3f", d3);
        json_object *jdouble3 = json_object_new_double_s(d3, tmp);

        sprintf(dateT, "%d-%02d-%02d",p_weather->date.tm_year+1900, p_weather->date.tm_mon+1, p_weather->date.tm_mday);
        json_object *jstring2 = json_object_new_string(dateT);

        json_object * jobj2 = json_object_new_object();
        
        json_object_object_add(jobj2,"Temperatura maxima", jdouble1);
        json_object_object_add(jobj2,"Temperatura minima", jdouble2);
        json_object_object_add(jobj2,"Precipitacion", jdouble3);
        json_object_object_add(jobj2,"Fecha", jstring2);
        
        /* adding the above created json strings to the array */
        json_object_array_add(jarray,jobj2);        

        p_weather++;
    }

    /* form the json object */
    /* each of these is like a key value pair */
    json_object_object_add(jobj,"Ciudad", jstring);
    json_object_object_add(jobj,"Pronostico extendido", jarray);

    /* now return the json object */
    return (unsigned char *)json_object_to_json_string(jobj);
}


/**
* Return a JSON with a list of the cities of the csv file.
*/
unsigned char * list_of_cities_JSON()
{
    char *list_cities;

    string_init(&list_cities);
    
    struct weatherData *p_weather;
    
    /* pointer to first element of weather structure array */
    p_weather = weather;
    
    /* creating a json object */    
    json_object * jobj = json_object_new_object(); 

    /* creating a json array */
    json_object *jarray = json_object_new_array();

    size_t count = 0;
    size_t i;

    /* Loop to get a non-repeating list of cities */
    for (i = 0; i < count_weather; i++)
    {
        /* check if p_weather->city is in arr_cities */
        if (strstr(list_cities, p_weather->city) != NULL) {            
            p_weather++;
            continue;
        }
        
        string_set(&list_cities, p_weather->city);

        /* add the value of p_weather->city into the arr_cities */
        strcat(list_cities, p_weather->city);

        /* creating a json string */
        json_object *jstring = json_object_new_string(p_weather->city);        
        
        /* adding the above created json strings to the array */
        json_object_array_add(jarray,jstring);        

        /* next element */
        p_weather++;

    }

    string_free(&list_cities);

    /* form the json object */
    /* each of these is like a key value pair */
    json_object_object_add(jobj,"cities", jarray);

    printf("%s\n", json_object_to_json_string(jobj));

    /* now return the json object */
    return (unsigned char *)json_object_to_json_string(jobj);
}


/**
* Save the values of the GET request in global variables.
*/
int save_get_values (void *cls, enum MHD_ValueKind kind, const char *key,
               const char *value)
{
    (void) cls;    /* Unused. Silent compiler warning. */
    (void) kind;   /* Unused. Silent compiler warning. */

    if (!strcmp(key,"date"))
        strcpy(p_date, value);

    if (!strcmp(key,"city"))
        strcpy(p_city, value);
    

    if (!strcmp(key,"unit"))
        strcpy(p_unit, value);

    return MHD_YES;
}


/**
* Verify the GET request and queue a response to be transmitted to the client.
*/
int answer_to_connection (void *cls, struct MHD_Connection *connection,
                          const char *url,
                          const char *method, const char *version,
                          const char *upload_data,
                          size_t *upload_data_size, void **con_cls)
{    
    static int dummy;
    
    /* accept only GET request*/
    if (0 != strcmp (method, "GET"))
        return MHD_NO;
    
    /* upload data in a GET!? */
    if (0 != *upload_data_size)
        return MHD_NO;

    char *page;
    
    /* define route for API cities */
    if (strcmp("/api/cities",url) == 0)
    {
        page  = list_of_cities_JSON();
    }

    /* define route for API search */
    if (strcmp("/api/search",url) == 0)
    {
        MHD_get_connection_values (connection, MHD_GET_ARGUMENT_KIND, save_get_values, NULL);
        get_weather_for_city_and_date();
        page  = weather_for_city_and_date_JSON();
    }
    
    struct MHD_Response *response;
    int ret;

    /* creating the response*/
    response = MHD_create_response_from_buffer (strlen (page), (void*) page, MHD_RESPMEM_PERSISTENT);
    
    char *client = "http://localhost:9000";
    /* to allow CORS from origin. Character "*" allows all origins (not recommended) */
    MHD_add_response_header(response, "Access-Control-Allow-Origin", client);
    
    /* queue a response to be transmitted to the client */
    ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
    
    MHD_destroy_response (response);

    return ret;
}


 /** 
  * Initial memory allocation 
  */
void string_init(char** string_ptr)
{    
    /* create a string pointer and allocate memory for it */
    char *ptr = malloc(sizeof(char));

    /* dereference our pointer and set its address to the new contiguous block of memory */
    *string_ptr = ptr;
}


 /** 
  * Free memory
  */
void string_free(char** string_ptr)
{
    free(*string_ptr);
}


/** 
 * Reallocating memory 
*/
void string_set(char** destination, char* value)
{
    int new_size = strlen(value);
    *destination = realloc(*destination, sizeof(char)*new_size+1);
}