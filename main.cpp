#include "mbed.h"
#include <cmath>

AnalogIn apps_0{PA_4};
AnalogIn apps_1{PA_5};
Timer timer;

bool sensors_diff_threshold_crossed;
bool program_running = true;

const double LUT[5][2] = {
    {0.0, 0.0},
    {0.1, 0.3},
    {0.5, 0.7},
    {0.9, 0.9},
    {1, 0.95}};

double getThrottleMapping(double x)
{
   int lut_0, lut_1;
   for (int i = 0; i < 5; i++)
   {
      if (x == LUT[i][0])
      {
         return LUT[i][1];
      }
      else if (x > LUT[i][0])
      {
         lut_0 = i;
         lut_1 = i + 1;
      }
      else
      {
         break;
      }
   }

   return LUT[lut_0][1] + (LUT[lut_1][1] - LUT[lut_0][1]) * ((x - LUT[lut_0][0]) / (LUT[lut_1][0] - LUT[lut_0][0]));
}

const double pedal_intercept_1 = -0.25;
const double pedal_intercept_2 = -0.3;
const double pedal_scale_1 = 0.5;
const double pedal_scale_2 = (0.5 / 1.2);

int main()
{
   while (true)
   {
      if (program_running)
      {
         // printf("Hello World!\n");

         // pedal position in percentage from each
         double pedal_pos_1 = pedal_scale_1 * (apps_0.read() - pedal_intercept_1);
         double pedal_pos_2 = pedal_scale_2 * (apps_1.read() - pedal_intercept_2);

         // average them
         double avg_pos = (pedal_pos_1 + pedal_pos_2) / 2;

         // Get if it is traveling (10% diff)
         sensors_diff_threshold_crossed = (abs(pedal_pos_1 - pedal_pos_2)) / ((pedal_pos_1 + pedal_pos_2) / 2) > 0.1;

         // If traveling and timer has not started, start it
         if (sensors_diff_threshold_crossed && (timer.elapsed_time().count() == 0))
         {
            timer.start();
         }
         // If traveling and timer has started and has exceeded 100ms, cut power
         else if (sensors_diff_threshold_crossed && (timer.elapsed_time().count() / 1000) >= 100)
         {
            printf("0\n");
            program_running = false;
         }
         // If timer is running and we stopped traveling, then stop and reset the timer
         else if (!sensors_diff_threshold_crossed && (timer.elapsed_time().count()) > 0)
         {
            timer.stop();
            timer.reset();
         }

         // Print pedal position
         // printf("%f%%\n", avg_pos);

         // Print pedal position (with LUT)
         printf("%f%%\n", getThrottleMapping(avg_pos));
      }
   }

   // main() is expected to loop forever.
   // If main() actually returns the processor will halt
   return 0;
}