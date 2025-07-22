#include "data_logger.h"
#include "stm32f10x.h"
#include "string.h"
#include "stdio.h"

DataLoggerInfo_t logger_info;

static u8 DataLogger_WriteRecord(LogRecord_t* record);
static u8 DataLogger_ReadRecord(u32 addr, LogRecord_t* record);
static u8 DataLogger_ErasePage(u16 page);
static u32 DataLogger_GetTimestamp(void);

u8 DataLogger_Init(void)
{
    u32 addr;
    LogRecord_t record;
    u16 page_records;
    u16 page;
    u16 offset;

    printf("Initializing data logger...\r\n");
    memset(&logger_info, 0, sizeof(logger_info));

    for(page = 0; page < (FLASH_END_ADDR - FLASH_START_ADDR + 1) / FLASH_PAGE_SIZE; page++)
    {
        page_records = 0;
        for(offset = 0; offset < LOG_RECORDS_PER_PAGE; offset++)
        {
            addr = FLASH_START_ADDR + page * FLASH_PAGE_SIZE + offset * LOG_RECORD_SIZE;
            if(DataLogger_ReadRecord(addr, &record) == 0)
            {
                if(record.raw_data[0] != 0xFF)
                {
                    page_records++;
                    logger_info.total_records++;
                    switch(record.sensor.log_type)
                    {
                        case LOG_TYPE_SENSOR:    logger_info.sensor_records++;    break;
                        case LOG_TYPE_OPERATION: logger_info.operation_records++; break;
                        case LOG_TYPE_ALARM:     logger_info.alarm_records++;     break;
                    }
                    if(logger_info.oldest_timestamp == 0 || record.sensor.timestamp < logger_info.oldest_timestamp)
                        logger_info.oldest_timestamp = record.sensor.timestamp;
                    if(record.sensor.timestamp > logger_info.newest_timestamp)
                        logger_info.newest_timestamp = record.sensor.timestamp;
                    logger_info.current_page = page;
                    logger_info.current_offset = offset + 1;
                }
                else
                {
                    goto end_scan;
                }
            }
        }
    }
end_scan:
    if(logger_info.current_offset >= LOG_RECORDS_PER_PAGE)
    {
        logger_info.current_page++;
        logger_info.current_offset = 0;
    }
    printf("Data logger initialized. Records: %lu\r\n", logger_info.total_records);
    return 0;
}

static u32 DataLogger_GetTimestamp(void)
{
    return RTC_GetCounter();
}

static u8 DataLogger_WriteRecord(LogRecord_t* record)
{
    u32 addr;
    FLASH_Status status;
    u32* data_ptr = (u32*)record->raw_data;
    u8 i;

    if(logger_info.current_offset >= LOG_RECORDS_PER_PAGE)
    {
        logger_info.current_page++;
        logger_info.current_offset = 0;
    }
    if(logger_info.current_page >= (FLASH_END_ADDR - FLASH_START_ADDR + 1) / FLASH_PAGE_SIZE)
    {
        printf("Flash full, erasing page 0.\r\n");
        DataLogger_ErasePage(0);
        logger_info.current_page = 0;
        logger_info.current_offset = 0;
    }

    addr = FLASH_START_ADDR + logger_info.current_page * FLASH_PAGE_SIZE + 
           logger_info.current_offset * LOG_RECORD_SIZE;

    if(logger_info.current_offset == 0)
    {
        if(*(volatile u32*)(addr) != 0xFFFFFFFF)
        {
            DataLogger_ErasePage(logger_info.current_page);
        }
    }

    FLASH_Unlock();
    for(i = 0; i < LOG_RECORD_SIZE / 4; i++)
    {
        status = FLASH_ProgramWord(addr + i * 4, data_ptr[i]);
        if(status != FLASH_COMPLETE)
        {
            printf("Flash write failed!\r\n");
            FLASH_Lock();
            return 1;
        }
    }
    FLASH_Lock();
    logger_info.current_offset++;
    logger_info.total_records++;
    return 0;
}

static u8 DataLogger_ReadRecord(u32 addr, LogRecord_t* record)
{
    u8 i;
    u32* data_ptr = (u32*)record->raw_data;
    u32* flash_ptr = (u32*)addr;
    if(addr < FLASH_START_ADDR || addr > FLASH_END_ADDR - LOG_RECORD_SIZE) return 1;
    for(i = 0; i < LOG_RECORD_SIZE / 4; i++)
    {
        data_ptr[i] = flash_ptr[i];
    }
    return 0;
}

static u8 DataLogger_ErasePage(u16 page)
{
    u32 page_addr = FLASH_START_ADDR + page * FLASH_PAGE_SIZE;
    FLASH_Status status;
    FLASH_Unlock();
    status = FLASH_ErasePage(page_addr);
    FLASH_Lock();
    if(status != FLASH_COMPLETE)
    {
        printf("Page erase failed!\r\n");
        return 1;
    }
    return 0;
}

u8 DataLogger_WriteSensorData(u8 temp, u8 humi, u8 light, 
                              u8 fan, u8 pump, u8 light_dev, 
                              u8 mode, u8 alarms)
{
    LogRecord_t record;
    memset(&record, 0, sizeof(record));
    record.sensor.timestamp = DataLogger_GetTimestamp();
    record.sensor.log_type = LOG_TYPE_SENSOR;
    record.sensor.temperature = temp;
    record.sensor.humidity = humi;
    record.sensor.light = light;
    record.sensor.fan_status = fan;
    record.sensor.pump_status = pump;
    record.sensor.light_status = light_dev;
    record.sensor.work_mode = mode;
    record.sensor.alarm_flags = alarms;
    if(DataLogger_WriteRecord(&record) == 0)
    {
        logger_info.sensor_records++;
        logger_info.newest_timestamp = record.sensor.timestamp;
        return 0;
    }
    return 1;
}

u8 DataLogger_WriteOperation(u8 operation, u8 old_val, u8 new_val, u8 trigger)
{
    LogRecord_t record;
    memset(&record, 0, sizeof(record));
    record.operation.timestamp = DataLogger_GetTimestamp();
    record.operation.log_type = LOG_TYPE_OPERATION;
    record.operation.operation = operation;
    record.operation.old_value = old_val;
    record.operation.new_value = new_val;
    record.operation.trigger_mode = trigger;
    if(DataLogger_WriteRecord(&record) == 0)
    {
        logger_info.operation_records++;
        logger_info.newest_timestamp = record.operation.timestamp;
        return 0;
    }
    return 1;
}

u8 DataLogger_WriteAlarm(u8 alarm_type, u8 level, u8 trigger_val, u8 threshold)
{
    LogRecord_t record;
    memset(&record, 0, sizeof(record));
    record.alarm.timestamp = DataLogger_GetTimestamp();
    record.alarm.log_type = LOG_TYPE_ALARM;
    record.alarm.alarm_type = alarm_type;
    record.alarm.alarm_level = level;
    record.alarm.trigger_value = trigger_val;
    record.alarm.threshold = threshold;
    if(DataLogger_WriteRecord(&record) == 0)
    {
        logger_info.alarm_records++;
        logger_info.newest_timestamp = record.alarm.timestamp;
        return 0;
    }
    return 1;
}

u16 DataLogger_Query(LogQuery_t* query, LogRecord_t* records, u16 buffer_size)
{
    u16 found_count = 0;
    u32 addr;
    LogRecord_t record;
    u16 page, offset;

    for(page = logger_info.current_page; ; page--)
    {
        u16 start_offset = (page == logger_info.current_page) ? logger_info.current_offset - 1 : LOG_RECORDS_PER_PAGE - 1;
        for(offset = start_offset; ; offset--)
        {
            addr = FLASH_START_ADDR + page * FLASH_PAGE_SIZE + offset * LOG_RECORD_SIZE;
            if(DataLogger_ReadRecord(addr, &record) == 0)
            {
                if(record.raw_data[0] != 0xFF)
                {
                    if(record.sensor.timestamp >= query->start_time && record.sensor.timestamp <= query->end_time)
                    {
                        if(query->log_type == 0 || record.sensor.log_type == query->log_type)
                        {
                            if(found_count < buffer_size && found_count < query->max_records)
                            {
                                records[found_count] = record;
                                found_count++;
                            }
                            else
                            {
                                return found_count;
                            }
                        }
                    }
                }
            }
            if(offset == 0) break;
        }
        if(page == 0) break;
    }
    return found_count;
}

u8 DataLogger_GetInfo(DataLoggerInfo_t* info)
{
    *info = logger_info;
    return 0;
}

u8 DataLogger_EraseAll(void)
{
    u16 page;
    printf("Erasing all data...\r\n");
    for(page = 0; page < (FLASH_END_ADDR - FLASH_START_ADDR + 1) / FLASH_PAGE_SIZE; page++)
    {
        if(DataLogger_ErasePage(page) != 0)
        {
            printf("Failed to erase page %d\r\n", page);
            return 1;
        }
    }
    memset(&logger_info, 0, sizeof(logger_info));
    printf("All data cleared.\r\n");
    return 0;
}

u8 DataLogger_GetDailyStats(u32 date, u8* avg_temp, u8* avg_humi, 
                           u8* avg_light, u16* operations)
{
    LogQuery_t query;
    LogRecord_t records[100];
    u16 found;
    u32 temp_sum = 0, humi_sum = 0, light_sum = 0;
    u16 sensor_count = 0;
    u16 i;
    
    query.start_time = date;
    query.end_time = date + 86400;
    query.log_type = 0;
    query.max_records = 100;
    
    found = DataLogger_Query(&query, records, 100);
    *operations = 0;
    
    for(i = 0; i < found; i++)
    {
        if(records[i].sensor.log_type == LOG_TYPE_SENSOR)
        {
            temp_sum += records[i].sensor.temperature;
            humi_sum += records[i].sensor.humidity;
            light_sum += records[i].sensor.light;
            sensor_count++;
        }
        else if(records[i].operation.log_type == LOG_TYPE_OPERATION)
        {
            (*operations)++;
        }
    }
    
    if(sensor_count > 0)
    {
        *avg_temp = temp_sum / sensor_count;
        *avg_humi = humi_sum / sensor_count;
        *avg_light = light_sum / sensor_count;
    }
    else
    {
        *avg_temp = 0;
        *avg_humi = 0;
        *avg_light = 0;
    }
    return 0;
}

void DataLogger_PrintRecords(u16 count)
{
    LogQuery_t query;
    LogRecord_t records[20];
    u16 found, i;
    RTC_Time_t time;
    
    printf("--- Last %d Records ---\r\n", count);
    
    query.start_time = 0;
    query.end_time = 0xFFFFFFFF;
    query.log_type = 0;
    query.max_records = count > 20 ? 20 : count;
    
    found = DataLogger_Query(&query, records, 20);
    
    for(i = 0; i < found; i++)
    {
        RTC_SetCounter(records[i].sensor.timestamp);
        RTC_Get_Time(&time);
        
        printf("[%04d-%02d-%02d %02d:%02d:%02d] ", 
               time.year, time.month, time.date, 
               time.hour, time.min, time.sec);
        
        switch(records[i].sensor.log_type)
        {
            case LOG_TYPE_SENSOR:
                printf("Sensor: T=%dC H=%d%% L=%d%% F=%d P=%d Li=%d M=%d A=0x%02X\r\n",
                       records[i].sensor.temperature, records[i].sensor.humidity,
                       records[i].sensor.light, records[i].sensor.fan_status,
                       records[i].sensor.pump_status, records[i].sensor.light_status,
                       records[i].sensor.work_mode, records[i].sensor.alarm_flags);
                break;
            case LOG_TYPE_OPERATION:
                printf("Operation: OP=%d %d->%d Trig=%d\r\n",
                       records[i].operation.operation, records[i].operation.old_value,
                       records[i].operation.new_value, records[i].operation.trigger_mode);
                break;
            case LOG_TYPE_ALARM:
                printf("Alarm: Type=%d Lvl=%d Val=%d Thres=%d\r\n",
                       records[i].alarm.alarm_type, records[i].alarm.alarm_level,
                       records[i].alarm.trigger_value, records[i].alarm.threshold);
                break;
            default:
                printf("Unknown type: %d\r\n", records[i].sensor.log_type);
                break;
        }
    }
    printf("==================\r\n");
}
