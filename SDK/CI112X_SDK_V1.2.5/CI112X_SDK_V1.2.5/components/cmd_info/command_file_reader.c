﻿
#include "stdio.h"
#include "ci112x_spiflash.h"
#include "flash_rw_process.h"
#include "command_file_reader.h"


#pragma pack(push)
#pragma pack(1)


#define MAX_CMD_FILE_FLAG_LEN      16      //!Max length of flag string of command information file.

#define cmd_file_reader_malloc(x)          pvPortMalloc(x)
#define cmd_file_reader_free(x)            vPortFree(x)

/**
 * @brief Command information file header.
 * 
 */
typedef struct cmd_file_header_st
{
	uint16_t file_format_version;               //!Version of file format.
    char file_flag[MAX_CMD_FILE_FLAG_LEN];      //!Flag of command informatin file.
	uint8_t model_number;                       //!Model number.
}cmd_file_header_t;

typedef struct cmd_models_table_item_st
{
    uint8_t model_group_id;
    uint32_t model_group_offset;
}cmd_models_table_item_t;

/**
 * @brief Model header
 * 
 */
typedef struct model_header_st
{
	uint16_t command_number;                        //!Number of command.    
	uint16_t dnn_id;                                //!ID of DNN file.
	uint16_t asr_id;                                //!ID of ASR file.
	uint16_t voice_group_id;                        //!ID of voice group.
	uint32_t command_table_offset;                  //!Start offset of command table in file.
	uint16_t special_word_number;                   //!Number of special words.
	uint32_t special_word_wait_count_table_offset;  //!Start offset of table that save the wait time after special word.
	uint32_t cmd_string_index_table_offset;         //!Start offset of table that save the index of strings.
	uint32_t cmd_string_table_size;                 //!Size of table that save the command strings.
	uint32_t cmd_string_table_offset;               //!Start offset of table that save the command strings.
	uint16_t voice_number;                          //!Number of voice.
	uint32_t voice_table_offset;                    //!Start offset of table that save the voice.
}model_header_t;

#pragma pack(pop)


static uint32_t cmd_file_start_addr_in_flash = (uint32_t)-1;
static cmd_file_header_t cmd_file_header = {0};
static model_header_t model_header ={0};


uint32_t cmd_file_reader_init(uint32_t start_addr_in_flash)
{
	cmd_file_start_addr_in_flash = start_addr_in_flash;
	uint32_t offset = start_addr_in_flash;
	
	post_read_flash((char*)&cmd_file_header, offset, sizeof(cmd_file_header));
	offset += sizeof(cmd_file_header);
    return 0;
}


uint32_t cmd_file_get_model_number()
{
	return cmd_file_header.model_number;
}


uint32_t cmd_file_change_cur_model_group(uint8_t model_group_id)
{
    uint32_t models_table_size = sizeof(cmd_models_table_item_t)*cmd_file_header.model_number;
    cmd_models_table_item_t *p_cmd_models_table = (cmd_models_table_item_t *)cmd_file_reader_malloc(models_table_size);
    if (p_cmd_models_table)
    {
        post_read_flash((char*)p_cmd_models_table, cmd_file_start_addr_in_flash + sizeof(cmd_file_header), models_table_size);
        int i = 0;
        for (i = 0;i < cmd_file_header.model_number;i++)
        {
            if (p_cmd_models_table[i].model_group_id == model_group_id)
            {
                post_read_flash((char*)&model_header, cmd_file_start_addr_in_flash + p_cmd_models_table[i].model_group_offset, sizeof(model_header));
                break;
            }
        }
        if (i > cmd_file_header.model_number)
            ci_logerr(LOG_USER, "error model id\n");
        cmd_file_reader_free(p_cmd_models_table);
        return 0;
    }
    else
    {
        return 1;
    }
}

uint32_t cmd_file_get_cur_model_dnn_id()
{
    return model_header.dnn_id;
}

uint32_t cmd_file_get_cur_model_id(uint32_t *p_dnn_id, uint32_t *p_asr_id, uint32_t *p_voice_group_id)
{
    if (p_dnn_id)
    {
        *p_dnn_id = model_header.dnn_id;
    }
    if (p_asr_id)
    {
        *p_asr_id = model_header.asr_id;
    }
    if (p_voice_group_id)
    {
        *p_voice_group_id = model_header.voice_group_id;
    }
    return 0;
}

uint32_t cmd_file_get_command_number()
{
	return model_header.command_number;
}

uint32_t cmd_file_read_command_table(void* table_buffer)
{
	uint32_t offset = cmd_file_start_addr_in_flash + model_header.command_table_offset;
	uint32_t size = sizeof(command_info_t)*model_header.command_number;
	post_read_flash((char*)table_buffer, offset, size);
	return 0;
}

uint32_t cmd_file_get_special_word_number()
{
	return model_header.special_word_number;
}

uint32_t cmd_file_read_special_word_table(void* table_buffer)
{
	uint32_t offset = cmd_file_start_addr_in_flash + model_header.special_word_wait_count_table_offset;
	uint32_t size = sizeof(special_wait_count_t)*model_header.special_word_number;
	post_read_flash((char*)table_buffer, offset, size);
	return 0;
}

uint32_t cmd_file_read_string_index_table(void* table_buffer)
{
	uint32_t offset = cmd_file_start_addr_in_flash + model_header.cmd_string_index_table_offset;
	uint32_t size = sizeof(command_string_index_t)*model_header.command_number;
	post_read_flash((char*)table_buffer, offset, size);
	return 0;
}


uint32_t cmd_file_get_string_table_size()
{
	return model_header.cmd_string_table_size;
} 

uint32_t cmd_file_read_string_table(void* table_buffer)
{
	uint32_t offset = cmd_file_start_addr_in_flash + model_header.cmd_string_table_offset;
	uint32_t size = model_header.cmd_string_table_size;
	post_read_flash((char*)table_buffer, offset, size);
	return 0;
}

uint32_t cmd_file_get_voice_number()
{
	return model_header.voice_number;
}

uint32_t cmd_file_read_voice_table(void* table_buffer) 
{
	uint32_t offset = cmd_file_start_addr_in_flash + model_header.voice_table_offset;
	uint32_t size = sizeof(voice_info_t)*model_header.voice_number;
	post_read_flash((char*)table_buffer, offset, size);
    return 0;
}









