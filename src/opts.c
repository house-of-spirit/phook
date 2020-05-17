#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "opts.h"
#include "lib/cjson/cJSON.h"

static void print_opts(opts_t*);

opts_t *read_opts_file(char* filename)
{
    size_t  file_len;
    FILE*   handle;
    opts_t* result;

    if( (handle = fopen(filename, "r")) == NULL)
    {
        perror("fopen");
        exit(1);
    }
    
    setvbuf(handle, NULL, _IONBF, 0);
    
    fseek(handle, 0, SEEK_END);
    file_len = ftello(handle);
    
    rewind(handle);

    char *contents = malloc(file_len + 1);

    contents[ fread(contents, 1, file_len, handle) ] = '\x00';
    result = read_opts_json(contents);

    free(contents);
    
    return result;
}

opts_t *read_opts_json(char* json_buf)
{
    cJSON*  parsed;
    cJSON*  target;
    cJSON*  exec_list;
    cJSON*  env;

    size_t  iterator_idx;
    cJSON*  iterator;
    
    cJSON*  acc;

    opts_t* result;
    
    #define parsing_error(err, ...) \
        fprintf(stderr, "An error occurred parsing JSON: " err, ##__VA_ARGS__); \
        exit(1);

    if ( (parsed = cJSON_Parse(json_buf)) == NULL)
    {
        parsing_error("Invalid JSON file.\n");
    }
    
    if ( !cJSON_IsObject(parsed))
    {
        parsing_error("Root element not [object].\n");
    }

    result = calloc(1, sizeof *result);
    
    if(!cJSON_IsObject(target = cJSON_GetObjectItemCaseSensitive(parsed, "target_executable")))
    {
        parsing_error("\"target_executable\" either missing or not [object].\n");
    }
    
    exec_list = cJSON_GetObjectItemCaseSensitive(target, "exec");

    if(cJSON_IsString(exec_list))
    {
        result->target_executable.path = strdup(exec_list->valuestring);
        
        result->target_executable.argv = malloc(sizeof(char*) * 2);
        result->target_executable.argv[0] = strdup(exec_list->valuestring);
        result->target_executable.argv[1] = NULL;
    }

    else if (cJSON_IsArray(exec_list))
    {
        iterator_idx = 0;
        
        /* Enough space for all elements plus a NULL terminator */
        result->target_executable.argv = malloc( (cJSON_GetArraySize(exec_list) + 1) * sizeof(char*) );

        cJSON_ArrayForEach(iterator, exec_list)
        {
            if(!cJSON_IsString(iterator))
            {
                parsing_error("\"exec\": element `%lu` not [string].\n", iterator_idx);
            }
            
            result->target_executable.argv[iterator_idx] = strdup(iterator->valuestring);
            iterator_idx++;
        }
        
        if(iterator_idx == 0)
        {
            parsing_error("\"exec\": empty array.\n");
        }
        
        result->target_executable.argv[iterator_idx] = NULL;
        result->target_executable.path = strdup(result->target_executable.argv[0]);

    }

    else
    {
        parsing_error("\"exec\" either missing or not [string, array].\n");
    }
    
    env = cJSON_GetObjectItemCaseSensitive(target, "env");
    
    if(!env)
    {
        result->target_executable.envp = NULL;
    }

    else if(cJSON_IsObject(env))
    {
        iterator_idx = 0;
        
        /* Enough space for all elements plus a NULL terminator */
        result->target_executable.envp = malloc( (cJSON_GetArraySize(env) + 1) * sizeof(char*) );
        
        cJSON_ArrayForEach(iterator, env)
        {
            /* Iterator is guaranteed to be a string */
            
            acc = cJSON_GetObjectItemCaseSensitive(env, iterator->string);
            
            if(!cJSON_IsString(acc))
            {
                parsing_error("\"env\": element `%lu` not [string].\n", iterator_idx);
            }
            
            /* env vars are passed to program as single char* like:
             * "KEY=VALUE"
             * We'll paste the two together with a '=' */
            
            size_t key_length   = strlen(iterator->string), 
                   value_length = strlen(acc->valuestring);
            
            /* key length + length of '=' (1) + value length + length of '\0' (1) */
            result->target_executable.envp[iterator_idx] = malloc(key_length + value_length + 2);

            strcpy(result->target_executable.envp[iterator_idx], iterator->string);
            result->target_executable.envp[iterator_idx][key_length] = '=';
            strcpy(&result->target_executable.envp[iterator_idx][key_length + 1], acc->valuestring);
            
            iterator_idx++;
        }
        
        result->target_executable.envp[iterator_idx] = NULL;
    }

    else
    {
        parsing_error("\"env\": not [object]\n");
    }
    

    acc = cJSON_GetObjectItemCaseSensitive(target, "to_inject");

    if(!cJSON_IsString(acc))
    {
        parsing_error("\"to_inject\": either missing or not [string]\n");
    }
    
    result->to_inject_path = strdup(acc->valuestring);
    
    cJSON_Delete(parsed);

    print_opts(result);
    return result;

    #undef parse_error
}


static void print_opts(opts_t *opts)
{
    size_t i;

    puts("<><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><>");

    printf("target path:  \"%s\"\n", opts->target_executable.path);
    printf("argv:\n");
    
    i = 0;
    if(opts->target_executable.argv)
        while(opts->target_executable.argv[i])
            printf("\t\"%s\"\n", opts->target_executable.argv[i++]);
    

    printf("envp:\n");
    i = 0;

    if(opts->target_executable.envp)
        while(opts->target_executable.envp[i])
            printf("\t\"%s\"\n", opts->target_executable.envp[i++]);
    
    putchar('\n');

    printf("lib to inject: \"%s\"\n", opts->to_inject_path ? opts->to_inject_path : "NONE");

    puts("<><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><>");
}
