#include "mbed.h"
#include <cmath>

// APPS Accelerator Pedal Sensors
AnalogIn apps_0{PA_3};
AnalogIn apps_1{PA_4};

// Brake Pedal Sensor
AnalogIn brake_apps{PA_6};

// Cockpit Switch
DigitalIn cockpit_switch{PA_7, PullDown};

// timers
Timer bse_implaus_timer;
Timer trac_control_timer;

// buzzer
DigitalOut buzzer{PA_8};

// represents whether accelerator sensors are >10% different
bool sensors_diff_threshold_crossed;

// Checks passed
bool brake_passed = true;
bool cockpit_passed = false;

// vars that set implaus or trac control on
bool bse_implaus = false;
bool trac_control = false;

// high braking threshold (for BSE)
const double high_brake_thresh = 0.9;
const double high_current_thresh = 0.8;

// LUT (must have 0.0 and 1.0 x vals)
// const double LUT[5][2] = {
//     {0.0, 0.0},
//     {0.1, 0.3},
//     {0.5, 0.7},
//     {0.9, 0.9},
//     {1.0, 0.95}};

// 1:1 LUT
const double LUT[2][2] = {
    {0.0, 0.0},
    {1.0, 1.0}};

// get percentage throttle from input percentage throttle (input being the linear %)
double getThrottleMapping(double x)
{
   // Find closest two values from LUT
   int lut_0, lut_1;
   for (int i = 0; i < sizeof(LUT) / sizeof(LUT[0]); i++)
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
   }

   // LERP
   return LUT[lut_0][1] + (LUT[lut_1][1] - LUT[lut_0][1]) * ((x - LUT[lut_0][0]) / (LUT[lut_1][0] - LUT[lut_0][0]));
}

// Pedal linear scales
const double pedal_intercept_1 = -0.25;
const double pedal_intercept_2 = -0.3;
const double brake_intercept = -0.25;
const double pedal_scale_1 = 0.5;
const double pedal_scale_2 = (0.5 / 1.2);
const double brake_scale = 0.5;

// "fake" rpms
double rpm_front = 140;
double rpm_back = 140;

int main()
{
   while (true)
   {
      if (brake_passed && cockpit_passed)
      {
         // brake pedal position
         double brake_pos = brake_scale * ((brake_apps.read() * 3.3) + brake_intercept);

         // Get accelerator pedals as percentages
         double pedal_pos_1 = pedal_scale_1 * ((apps_0.read() * 3.3) + pedal_intercept_1);
         double pedal_pos_2 = pedal_scale_2 * ((apps_1.read() * 3.3) + pedal_intercept_2);

         // average them
         double avg_pos = (pedal_pos_1 + pedal_pos_2) / 2;

         // Get if there might be implaus
         sensors_diff_threshold_crossed = (abs(pedal_pos_1 - pedal_pos_2)) / ((pedal_pos_1 + pedal_pos_2) / 2) > 0.1;
         if (pedal_pos_1 < 0 || pedal_pos_1 > 1 || pedal_pos_2 < 0 || pedal_pos_2 > 1)
         {
            sensors_diff_threshold_crossed = true;
         }

         // BSE high brake threshold but the car is running high current so cut power
         if (brake_pos > high_brake_thresh && avg_pos > high_current_thresh) {
            sensors_diff_threshold_crossed = true;
         }

         // If implaus and timer has not started, start it
         if (sensors_diff_threshold_crossed && (bse_implaus_timer.elapsed_time().count() == 0))
         {
            bse_implaus_timer.start();
         }
         // If implaus and timer has started and has exceeded 100ms, cut power
         else if (sensors_diff_threshold_crossed && (bse_implaus_timer.elapsed_time().count() / 1000) >= 100)
         {
            bse_implaus = true;
         }
         // If timer is running and we stopped implaus, then stop and reset the timer
         else if (!sensors_diff_threshold_crossed && (bse_implaus_timer.elapsed_time().count()) > 0)
         {
            bse_implaus = false;
            bse_implaus_timer.stop();
            bse_implaus_timer.reset();
         }

         // For traction control, if the front and back tires are spinning more than 10% difference in speeds
         double trac_difference_threshold_crossed = (abs(rpm_front - rpm_back)) / ((rpm_front + rpm_back) / 2) > 0.1;

         // start timer if trac control
         if (trac_difference_threshold_crossed && (trac_control_timer.elapsed_time().count() == 0))
         {
            trac_control_timer.start();
         }
         // If traction control and timer has started and has exceeded 100ms, cut power
         else if (trac_difference_threshold_crossed && (trac_control_timer.elapsed_time().count() / 1000) >= 100)
         {
            trac_control = true;
         }
         // If timer is running and we stopped traveling, then stop and reset the timer
         else if (!trac_difference_threshold_crossed && (trac_control_timer.elapsed_time().count()) > 0)
         {
            trac_control = false;
            trac_control_timer.stop();
            trac_control_timer.reset();
         }

         // Print pedal position
         if (!bse_implaus && !trac_control)
         {
            printf("%f%%\n", getThrottleMapping(avg_pos) * 100);
         }
         else
         {
            printf("0\n");
         }
      }
      else if (brake_passed)
      {
         if (cockpit_switch.read() == 1)
         {
            cockpit_passed = true;
         }
      }
      else
      {
         // Check if brakes are pressed at the beginning to start the car
         double brake_pos = brake_scale * ((brake_apps.read() * 3.3) + brake_intercept);

         // 80%
         if (brake_pos > 0.8)
         {
            brake_passed = true;

            // // Buzzer on
            // buzzer.write(1);

            // // Not sure if I should block thread or not??
            // ThisThread::sleep_for(500);

            // for debug
            printf("Brakes passed");

            // // buzzer off
            // buzzer.write(0);

            // // If need to print 0s before brake passing, I removed it so the console is cleaner
            // printf("0\n");
         }
      }
   }

   // main() is expected to loop forever.
   // If main() actually returns the processor will halt
   return 0;
}