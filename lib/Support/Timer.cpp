#include "Support/Timer.h"

void Timer::suspend() {
  if (!Suspended) {
    time(&SuspendStartTime);
    Suspended = true;
  }
}

void Timer::resume() {
  if (Suspended) {
    time_t CurrTime;
    time(&CurrTime);
    double TimeElapsed = difftime(CurrTime, SuspendStartTime);
    SuspendTime += TimeElapsed;
    Suspended = false;
  }
}

bool Timer::isTimeOut() {
  if (StepsCounter == 0) {
    StepsCounter = Steps;

    double TimeElapsed;

    if (Suspended) {
      TimeElapsed = difftime(SuspendStartTime, StartTime);
    } else {
      time_t CurrTime;
      time(&CurrTime);
      TimeElapsed = difftime(CurrTime, StartTime);
    }

    if (TimeElapsed > (Duration + SuspendTime)) {
      return true;
    }
  } else {
    StepsCounter--;
  }

  return false;
}

void Timer::check() {
  if (isTimeOut()) {
    TaskAfterTimeout();
  }
}