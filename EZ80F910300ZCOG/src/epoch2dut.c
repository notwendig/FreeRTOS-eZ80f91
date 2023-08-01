/*
EPOCH to human readable time and date converter for microcontrollers (PIC, AVR, Arduino, STM32)
Current EPOCH time can be found at https://www.epochconverter.com/
The code can be suitably modified to suit your requirements. I have extensively
tested this code with valid inputs. If you have any questions or if you find a bug, please contact me. 
Author: Siddharth Singh <sidsingh78@gmail.com>
Ver:1.0
Date: June 15, 2017 
*/

#include <stdlib.h>

static unsigned char month_days[12]={31,28,31,30,31,30,31,31,30,31,30,31};
static unsigned char week_days[7] = {4,5,6,0,1,2,3};
//Thu=4, Fri=5, Sat=6, Sun=0, Mon=1, Tue=2, Wed=3

unsigned char
ntp_hour, ntp_minute, ntp_second, ntp_week_day, ntp_date, ntp_month, leap_days, leap_year_ind ;

unsigned short temp_days;

unsigned int ntp_year, days_since_epoch, day_of_year; 

char key;


int epoch_to_date_time(time_t epoch)
{
   //---------------------------- Input and Calculations -------------------------------------

      int i;
    // Add or substract time zone here. 
    epoch+=3600 ; //GMT +1h
    
      ntp_second = epoch%60;
      epoch /= 60;
      ntp_minute = epoch%60;
      epoch /= 60;
      ntp_hour  = epoch%24;
      epoch /= 24;
        
      days_since_epoch = epoch;      //number of days since epoch
      ntp_week_day = week_days[days_since_epoch%7];  //Calculating WeekDay
      
      ntp_year = 1970+(days_since_epoch/365); // ball parking year, may not be accurate!
 
      for (i=1972; i<ntp_year; i+=4)      // Calculating number of leap days since epoch/1970
         if(((i%4==0) && (i%100!=0)) || (i%400==0)) leap_days++;
            
      ntp_year = 1970+((days_since_epoch - leap_days)/365); // Calculating accurate current year by (days_since_epoch - extra leap days)
      day_of_year = ((days_since_epoch - leap_days)%365)+1;
  
   
      if(((ntp_year%4==0) && (ntp_year%100!=0)) || (ntp_year%400==0))  
       {
         month_days[1]=29;     //February = 29 days for leap years
         leap_year_ind = 1;    //if current year is leap, set indicator to 1 
        }
            else month_days[1]=28; //February = 28 days for non-leap years 
   
            temp_days=0;
   
    for (ntp_month=0 ; ntp_month <= 11 ; ntp_month++) //calculating current Month
       {
           if (day_of_year <= temp_days) break; 
           temp_days = temp_days + month_days[ntp_month];
        }
    
    temp_days = temp_days - month_days[ntp_month-1]; //calculating current Date
    ntp_date = day_of_year - temp_days;
  
    return 0;
}
