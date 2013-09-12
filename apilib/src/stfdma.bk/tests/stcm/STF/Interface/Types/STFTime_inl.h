///
/// @file STF/Interface/Types/STFTime_inl.h
///
/// @brief  Inlines for STFTime.h
///
/// @par OWNER:
///             STFTeam
///
/// @author     Rolland Veres & Christian Johner
///
/// @par SCOPE:
///             Internal Header File
///
/// @date       2003-08-21 
///
/// &copy; 2003 STMicroelectronics Design and Applications. All Rights Reserved.
///

#ifndef STFTIME_INL_H
#define STFTIME_INL_H

////////////////////////////
// STFLoPrec32BitDuration
////////////////////////////

// Constructors
inline STFLoPrec32BitDuration::STFLoPrec32BitDuration (void) : duration (0)
   {
   }

inline STFLoPrec32BitDuration::STFLoPrec32BitDuration(int32 duration, STFTimeUnits units)
   {
   switch (units)
      {
      case STFTU_LOWSYSTEM:
         this->duration = duration;
         break;
      case STFTU_HIGHSYSTEM:
			duration = duration / LongTimeTicksPerShortTick;
         this->duration = ConvertClock108ToSystem(duration);
         break;
      case STFTU_MICROSECS:
         this->duration = duration / MicrosecsPerShortTimeTick;
         break;
      case STFTU_MILLISECS:
         this->duration = duration * ShortTimeTicksPerMillisec;
         break;
      case STFTU_SECONDS:
         this->duration = duration * ShortTimeTicksPerSecond;
         break;
		case STFTU_90KHZTICKS:
			this->duration = duration / 45; 
			break;
		case STFTU_108MHZTICKS:
			this->duration = duration / LongTimeTicksPerShortTick; 
			break;
		default:
			DP("You did choose a not existing TimeUnit for a STFLoPrec32BitDuration.");
			BREAKPOINT;
      }
   }


inline STFLoPrec32BitDuration::STFLoPrec32BitDuration(const STFLoPrec32BitDuration & duration)
   {
   this->duration =  duration.Get32BitDuration(STFTU_LOWSYSTEM);
   }

inline STFLoPrec32BitDuration::STFLoPrec32BitDuration(const STFHiPrec64BitDuration & duration)
   {
   this->duration =  duration.Get32BitDuration(STFTU_LOWSYSTEM);
   }

inline STFLoPrec32BitDuration::STFLoPrec32BitDuration(const STFHiPrec32BitDuration & duration)
   {
   this->duration =  duration.Get32BitDuration(STFTU_LOWSYSTEM);
   }

// Get32BitDuration
inline int32 STFLoPrec32BitDuration::Get32BitDuration(STFTimeUnits units) const
   {
   switch (units)
      {
      case STFTU_LOWSYSTEM:
         return duration;
         break;
      case STFTU_HIGHSYSTEM:
         return ConvertClock108ToSystem(duration * LongTimeTicksPerShortTick);
         break;
      case STFTU_MICROSECS:
         return duration * MicrosecsPerShortTimeTick;
         break;
      case STFTU_MILLISECS:
         return duration / ShortTimeTicksPerMillisec;
         break;
      case STFTU_SECONDS:
         return duration / ShortTimeTicksPerSecond;
         break;
		case STFTU_90KHZTICKS:
			return duration * 45;
			break;
		case STFTU_108MHZTICKS:
			return duration * LongTimeTicksPerShortTick;
			break;
      default:
			DP("You wanted to get a not existing TimeUnit for a STFLoPrec32BitDuration.");
			BREAKPOINT;
         return 0;
      }
	}

// Get64BitDuration
inline STFInt64 STFLoPrec32BitDuration::Get64BitDuration(STFTimeUnits units) const
   {
   switch (units)
      {
      case STFTU_LOWSYSTEM:
         return STFInt64(duration);
         break;
      case STFTU_HIGHSYSTEM:
         return ConvertClock108ToSystem(STFInt64(duration) * LongTimeTicksPerShortTick); //to have a lager amount of time we could multiply outside the brackets, but then the amount would be different to the Get32Bitdurations!!!
         break;
      case STFTU_MICROSECS:
         return STFInt64(duration) * MicrosecsPerShortTimeTick;
         break;
      case STFTU_MILLISECS:
         return STFInt64(duration / ShortTimeTicksPerMillisec);
         break;
      case STFTU_SECONDS:
         return STFInt64(duration / ShortTimeTicksPerSecond);
         break;
		case STFTU_90KHZTICKS:
			return STFInt64(duration) * 45;
			break;
		case STFTU_108MHZTICKS:
			return STFInt64(duration) * LongTimeTicksPerShortTick;
			break;
      default:
			DP("You wanted to get a not existing TimeUnit for a STFLoPrec32BitDuration.");
			BREAKPOINT;
         return 0;
      }
	}


// Assignment
inline const STFLoPrec32BitDuration & STFLoPrec32BitDuration::operator=(const STFLoPrec32BitDuration & duration)
   {
   this->duration = duration.Get32BitDuration(STFTU_LOWSYSTEM);
   
   return *this;
	}

inline const STFLoPrec32BitDuration & STFLoPrec32BitDuration::operator=(const STFHiPrec64BitDuration & duration)
   {
   this->duration = duration.Get32BitDuration(STFTU_LOWSYSTEM);
   
   return *this;
	}

inline const STFLoPrec32BitDuration & STFLoPrec32BitDuration::operator=(const STFHiPrec32BitDuration & duration)
   {
   this->duration = duration.Get32BitDuration(STFTU_LOWSYSTEM);
   
   return *this;
	}


// Increment
inline const STFLoPrec32BitDuration & STFLoPrec32BitDuration::operator+=(const STFLoPrec32BitDuration & duration)
   {
   this->duration += duration.Get32BitDuration(STFTU_LOWSYSTEM);
   
   return *this;
	}

inline const STFLoPrec32BitDuration & STFLoPrec32BitDuration::operator+=(const STFHiPrec64BitDuration & duration)
   {
   this->duration += duration.Get32BitDuration(STFTU_LOWSYSTEM);
   
   return *this;
	}

inline const STFLoPrec32BitDuration & STFLoPrec32BitDuration::operator+=(const STFHiPrec32BitDuration & duration)
   {
   this->duration += duration.Get32BitDuration(STFTU_LOWSYSTEM);
   
   return *this;
	}


// Decrement
inline const STFLoPrec32BitDuration & STFLoPrec32BitDuration::operator-=(const STFLoPrec32BitDuration & duration)
   {
   this->duration -= duration.Get32BitDuration(STFTU_LOWSYSTEM);
   
   return *this;
	}

inline const STFLoPrec32BitDuration & STFLoPrec32BitDuration::operator-=(const STFHiPrec64BitDuration & duration)
   {
   this->duration -= duration.Get32BitDuration(STFTU_LOWSYSTEM);
   
   return *this;
	}

inline const STFLoPrec32BitDuration & STFLoPrec32BitDuration::operator-=(const STFHiPrec32BitDuration & duration)
   {
   this->duration -= duration.Get32BitDuration(STFTU_LOWSYSTEM);
   
   return *this;
	}


// Difference
inline STFLoPrec32BitDuration STFLoPrec32BitDuration::operator-(const STFLoPrec32BitDuration & duration) const
   {
	return STFLoPrec32BitDuration(this->duration - duration.Get32BitDuration(STFTU_LOWSYSTEM), STFTU_LOWSYSTEM);
	}

inline STFLoPrec32BitDuration STFLoPrec32BitDuration::operator-(const STFHiPrec64BitDuration & duration) const
   {
	return STFLoPrec32BitDuration(this->duration - duration.Get32BitDuration(STFTU_LOWSYSTEM), STFTU_LOWSYSTEM);
	}

inline STFLoPrec32BitDuration STFLoPrec32BitDuration::operator-(const STFHiPrec32BitDuration & duration) const
   {
	return STFLoPrec32BitDuration(this->duration - duration.Get32BitDuration(STFTU_LOWSYSTEM), STFTU_LOWSYSTEM);
	}
	

// Summation
inline STFLoPrec32BitDuration STFLoPrec32BitDuration::operator+(const STFLoPrec32BitDuration & duration) const
   {
	return STFLoPrec32BitDuration(this->duration + duration.Get32BitDuration(STFTU_LOWSYSTEM), STFTU_LOWSYSTEM);
	}

inline STFLoPrec32BitDuration STFLoPrec32BitDuration::operator+(const STFHiPrec64BitDuration & duration) const
   {
	return STFLoPrec32BitDuration(this->duration + duration.Get32BitDuration(STFTU_LOWSYSTEM), STFTU_LOWSYSTEM);
	}

inline STFLoPrec32BitDuration STFLoPrec32BitDuration::operator+(const STFHiPrec32BitDuration & duration) const
   {
	return STFLoPrec32BitDuration(this->duration + duration.Get32BitDuration(STFTU_LOWSYSTEM), STFTU_LOWSYSTEM);
	}


// Summation with time		
inline STFHiPrec64BitTime STFLoPrec32BitDuration::operator+(const STFHiPrec64BitTime & time) const
   {
   return STFHiPrec64BitTime(this->Get64BitDuration(STFTU_108MHZTICKS) + time.Get64BitTime(STFTU_108MHZTICKS), STFTU_108MHZTICKS);
   }


// Equals
inline bool STFLoPrec32BitDuration::operator==(const STFLoPrec32BitDuration & duration) const
   {
   return this->duration == duration.Get32BitDuration(STFTU_LOWSYSTEM);
	}

inline bool STFLoPrec32BitDuration::operator==(const STFHiPrec64BitDuration & duration) const
   {
	return this->duration == duration.Get32BitDuration(STFTU_LOWSYSTEM);
	}

inline bool STFLoPrec32BitDuration::operator==(const STFHiPrec32BitDuration & duration) const
   {
	return this->duration == duration.Get32BitDuration(STFTU_LOWSYSTEM);
	}


// Equals NOT		
inline bool STFLoPrec32BitDuration::operator!=(const STFLoPrec32BitDuration & duration) const
	{
	return this->duration != duration.Get32BitDuration(STFTU_LOWSYSTEM);
	}
		
inline bool STFLoPrec32BitDuration::operator!=(const STFHiPrec64BitDuration & duration) const
	{
	return this->duration != duration.Get32BitDuration(STFTU_LOWSYSTEM);
	}
		
inline bool STFLoPrec32BitDuration::operator!=(const STFHiPrec32BitDuration & duration) const
	{
	return this->duration != duration.Get32BitDuration(STFTU_LOWSYSTEM);
	}


// Smaller or Equal		
inline bool STFLoPrec32BitDuration::operator<=(const STFLoPrec32BitDuration & duration) const
	{
	return this->duration <= duration.Get32BitDuration(STFTU_LOWSYSTEM);
	}
		
inline bool STFLoPrec32BitDuration::operator<=(const STFHiPrec64BitDuration & duration) const
	{
   return this->Get64BitDuration(STFTU_108MHZTICKS) <= duration.Get64BitDuration(STFTU_108MHZTICKS);
	}

inline bool STFLoPrec32BitDuration::operator<=(const STFHiPrec32BitDuration & duration) const
	{
   return this->duration <= duration.Get32BitDuration(STFTU_LOWSYSTEM);
   }


// Greater or Equal		
inline bool STFLoPrec32BitDuration::operator>=(const STFLoPrec32BitDuration & duration) const
	{
	return this->duration >= duration.Get32BitDuration(STFTU_LOWSYSTEM);
	}
		
inline bool STFLoPrec32BitDuration::operator>=(const STFHiPrec64BitDuration & duration) const
	{
	return this->duration >= duration.Get64BitDuration(STFTU_LOWSYSTEM);
	}
		
inline bool STFLoPrec32BitDuration::operator>=(const STFHiPrec32BitDuration & duration) const
	{
	return this->duration >= duration.Get32BitDuration(STFTU_LOWSYSTEM);
	}


// Smaller		
inline bool STFLoPrec32BitDuration::operator<(const STFLoPrec32BitDuration & duration) const
	{
	return this->duration < duration.Get32BitDuration(STFTU_LOWSYSTEM);
	}
		
inline bool STFLoPrec32BitDuration::operator<(const STFHiPrec64BitDuration & duration) const
	{
	return this->Get64BitDuration(STFTU_108MHZTICKS) < duration.Get64BitDuration(STFTU_108MHZTICKS);
	}
		
inline bool STFLoPrec32BitDuration::operator<(const STFHiPrec32BitDuration & duration) const
	{
	return this->duration < duration.Get32BitDuration(STFTU_LOWSYSTEM);
	}


// Greater		
inline bool STFLoPrec32BitDuration::operator>(const STFLoPrec32BitDuration & duration) const
	{
	return this->duration > duration.Get32BitDuration(STFTU_LOWSYSTEM);
	}
		
inline bool STFLoPrec32BitDuration::operator>(const STFHiPrec64BitDuration & duration) const
	{
	return this->duration > duration.Get32BitDuration(STFTU_LOWSYSTEM);
	}
		
inline bool STFLoPrec32BitDuration::operator>(const STFHiPrec32BitDuration & duration) const
	{
	return this->duration > duration.Get32BitDuration(STFTU_LOWSYSTEM);
	}


// Multiply
inline STFLoPrec32BitDuration STFLoPrec32BitDuration::operator*(const int32 factor) const
   {
   return STFLoPrec32BitDuration(this->duration * factor, STFTU_LOWSYSTEM);
   }


// Divide
inline STFLoPrec32BitDuration STFLoPrec32BitDuration::operator/(const int32 divider) const
   {
   return STFLoPrec32BitDuration(this->duration / divider, STFTU_LOWSYSTEM);
   }


// Scale
inline STFLoPrec32BitDuration STFLoPrec32BitDuration::Scale(const int32 from, const int32 to) const
   {
   uint32   res_u, res_l, upper, lower, udur, uto, ufrom;
   bool     sign;

   // Make sure that the divisor is NOT zero
   ASSERT(from != 0);
   if (from == 0) return STFLoPrec32BitDuration(0x7fffffff, STFTU_LOWSYSTEM);
   
   // avaluate sign of the result and continue the calculation with unsigned integers. 
   if (this->duration < 0)
      {
      udur = -this->duration;
      sign = true;
      }
   else
      {
      udur = this->duration;
      sign = false;
      }

   if (to < 0)
      {
      uto = -to;
      sign = !sign;
      }
   else
      {
      uto = to;
      }

   if (from < 0)
      {
      ufrom = -from;
      sign = !sign;
      }
   else
      {
      ufrom = from;
      }

   // Do the multiplication
   MUL32x32(udur, uto, upper, lower);
   
   // Do the division
   if (upper < ufrom)
      {
      res_u = 0;
      res_l = DIV64x32(upper, lower, ufrom);
      }
   else
      {
      res_u = upper / ufrom;
      res_l = DIV64x32( (upper % ufrom), lower, ufrom);
      }

   //return result depending on sign and possible saturation
   if (!sign) //positive case
      {
      if (res_u != 0 || res_l > 0x7fffffff) //positive saturation
         return STFLoPrec32BitDuration(0x7fffffff, STFTU_LOWSYSTEM);
      else //positive value
         return STFLoPrec32BitDuration(res_l, STFTU_LOWSYSTEM);
      }
   else //negative case
      {
      if (res_u != 0 || res_l > 0x80000000) //negative saturation
         return STFLoPrec32BitDuration(0x80000000, STFTU_LOWSYSTEM);
      else //negative value
         return STFLoPrec32BitDuration(-(int32)res_l, STFTU_LOWSYSTEM);
      }
   }


// FractMul
inline STFLoPrec32BitDuration STFLoPrec32BitDuration::FractMul(const int32 fract) const
   {
   uint32   res_l, res_u, udur, ufract;
   bool     sign;

   // evaluate sign of the result and continue the multiplication with unsigned integers.
   if (this->duration < 0)
      {
      udur = -this->duration;
      sign = true;
      }
   else
      {
      udur = this->duration;
      sign = false;
      }
   
   if (fract < 0)
      {
      ufract = -fract;
      sign = !sign;
      }
   else
      {
      ufract = fract;
      }

   // Do the multiplication
   MUL32x32(udur, ufract, res_u, res_l);
   
   // Shift result to compensate 16.16 fract
   res_l = (res_u << (32 - 16)) + (res_l >> 16);
   res_u = res_u >> 16;
   
   //return result depending on sign and possible saturation
   if (!sign) //positive case
      {
      if (res_u != 0 || res_l > 0x7fffffff) //positive saturation
         return STFLoPrec32BitDuration(0x7fffffff, STFTU_LOWSYSTEM);
      else //positive value
         return STFLoPrec32BitDuration(res_l, STFTU_LOWSYSTEM);
      }
   else //negative case
      {
      if (res_u != 0 || res_l > 0x80000000) //negative saturation
         return STFLoPrec32BitDuration(0x80000000, STFTU_LOWSYSTEM);
      else //negative value
         return STFLoPrec32BitDuration(-(int32)res_l, STFTU_LOWSYSTEM);
      }
   }


// FractDiv
inline STFLoPrec32BitDuration STFLoPrec32BitDuration::FractDiv(const int32 fract) const
   {
   uint32   res_l, res_u, lower, upper, udur, ufract;
   bool     sign;

   // Make sure that the divisor is NOT zero
   ASSERT(fract != 0);
   if (fract == 0) return STFLoPrec32BitDuration(0x7fffffff, STFTU_LOWSYSTEM);

   // evaluate sign of the result and continue the division with unsigned integers.
   if (this->duration < 0)
      {
      udur = -this->duration;
      sign = true;
      }
   else
      {
      udur = this->duration;
      sign = false;
      }
   
   if (fract < 0)
      {
      ufract = -fract;
      sign = !sign;
      }
   else
      {
      ufract = fract;
      }

   // shift the duration to compensate 16.16 fract
   upper = udur >> 16;
   lower = udur << 16;

   // Do the division
   if (upper < ufract)
      {
      res_u = 0;
      res_l = DIV64x32(upper, lower, ufract);
      }
   else
      {
      res_u = upper / ufract;
      res_l = DIV64x32( (upper % ufract), lower, ufract);
      }
   
   //return result depending on sign and possible saturation
   if (!sign) //positive case
      {
      if (res_u != 0 || res_l > 0x7fffffff) //positive saturation
         return STFLoPrec32BitDuration(0x7fffffff, STFTU_LOWSYSTEM);
      else //positive value
         return STFLoPrec32BitDuration(res_l, STFTU_LOWSYSTEM);
      }
   else //negative case
      {
      if (res_u != 0 || res_l > 0x80000000) //negative saturation
         return STFLoPrec32BitDuration(0x80000000, STFTU_LOWSYSTEM);
      else //negative value
         return STFLoPrec32BitDuration(-(int32)res_l, STFTU_LOWSYSTEM);
      }
   }


// Shift
inline STFLoPrec32BitDuration STFLoPrec32BitDuration::operator<<(const uint8 digits) const
   {
   return STFLoPrec32BitDuration(this->duration << digits, STFTU_LOWSYSTEM);
   }

inline STFLoPrec32BitDuration STFLoPrec32BitDuration::operator>>(const uint8 digits) const
   {
   return STFLoPrec32BitDuration(this->duration >> digits, STFTU_LOWSYSTEM);
   }


// Inside
inline bool STFLoPrec32BitDuration::Inside(const STFLoPrec32BitDuration & lower, const STFLoPrec32BitDuration & upper) const
	{
   return *this >= lower && *this <= upper;
   }

inline bool STFLoPrec32BitDuration::Inside(const STFHiPrec64BitDuration & lower, const STFHiPrec64BitDuration & upper) const
	{
   return *this >= lower && *this <= upper;
   }

inline bool STFLoPrec32BitDuration::Inside(const STFHiPrec32BitDuration & lower, const STFHiPrec32BitDuration & upper) const
	{
   return *this >= lower && *this <= upper;
   }


// GetFrequency
inline STFInt64 STFLoPrec32BitDuration::GetFrequency(void)
   {
   return ShortTimeTicksPerSecond;	// this value is fixed since the changeover to a 108MHZ base clock.
	}


//GetAbsolutDuration
inline STFLoPrec32BitDuration STFLoPrec32BitDuration::GetAbsoluteDuration(void) const
   {
   if (this->duration < 0) 
      return STFLoPrec32BitDuration((this->duration)*(-1));
   else 
      return STFLoPrec32BitDuration((this->duration));
   }


//////////////////////////
// STFHiPrec64BitDuration
//////////////////////////

// Constructors
inline STFHiPrec64BitDuration::STFHiPrec64BitDuration(void) : duration(0)
   {
	}

inline STFHiPrec64BitDuration::STFHiPrec64BitDuration(STFInt64 duration, STFTimeUnits units)
   {
	switch (units)
      {
      case STFTU_LOWSYSTEM:
         this->duration = duration * LongTimeTicksPerShortTick;
         break;
      case STFTU_HIGHSYSTEM:
         this->duration = ConvertClockSystemTo108(duration);
         break;
      case STFTU_MICROSECS:
         this->duration = duration * LongTimeTicksPerMicrosec;
         break;
      case STFTU_MILLISECS:
         this->duration = duration * LongTimeTicksPerMillisec;
         break;
      case STFTU_SECONDS:
         this->duration = duration * LongTimeTicksPerSecond;
         break;
		case STFTU_90KHZTICKS:
			this->duration = duration * 1200;
			break;
		case STFTU_108MHZTICKS:
			this->duration = duration; 
			break;
		default:
			DP("You did choose a not existing TimeUnit for a STFHiPrec64BitDuration.");
			BREAKPOINT;
      }
	}

inline STFHiPrec64BitDuration::STFHiPrec64BitDuration(const STFLoPrec32BitDuration & duration)
   {
   this->duration = duration.Get64BitDuration(STFTU_108MHZTICKS);
   }
      
inline STFHiPrec64BitDuration::STFHiPrec64BitDuration(const STFHiPrec64BitDuration & duration)
   {
   this->duration = duration.Get64BitDuration(STFTU_108MHZTICKS);
   }
      
inline STFHiPrec64BitDuration::STFHiPrec64BitDuration(const STFHiPrec32BitDuration & duration)
   {
   this->duration = duration.Get64BitDuration(STFTU_108MHZTICKS);
   }


// Get32BitDuration
inline int32 STFHiPrec64BitDuration::Get32BitDuration(STFTimeUnits units) const
   {
   switch (units)
      {
      case STFTU_LOWSYSTEM:
         return (duration / LongTimeTicksPerShortTick).ToInt32();
         break;
      case STFTU_HIGHSYSTEM:
         return (ConvertClock108ToSystem(duration)).ToInt32();
         break;
      case STFTU_MICROSECS:
         return (duration  / LongTimeTicksPerMicrosec).ToInt32();
         break;
      case STFTU_MILLISECS:
         return (duration / LongTimeTicksPerMillisec).ToInt32();
         break;
      case STFTU_SECONDS:
         return (duration / LongTimeTicksPerSecond).ToInt32();
         break;
		case STFTU_90KHZTICKS:
			return (duration / 1200).ToInt32();
			break;
		case STFTU_108MHZTICKS:
			return duration.ToInt32();
      default:
			DP("You wanted to get a not existing TimeUnit for a STFHiPrec64BitDuration.");
			BREAKPOINT;
         return 0;
      }
   }


// Get64BitDuration
inline STFInt64 STFHiPrec64BitDuration::Get64BitDuration(STFTimeUnits units) const
   {
   switch (units)
      {
      case STFTU_LOWSYSTEM:
         return duration / LongTimeTicksPerShortTick;
         break;
      case STFTU_HIGHSYSTEM:
         return ConvertClock108ToSystem(duration);
         break;
      case STFTU_MICROSECS:
         return duration  / LongTimeTicksPerMicrosec ;
         break;
      case STFTU_MILLISECS:
         return duration / LongTimeTicksPerMillisec;
         break;
      case STFTU_SECONDS:
         return duration / LongTimeTicksPerSecond;
         break;
		case STFTU_90KHZTICKS:
			return duration / 1200;
			break;
		case STFTU_108MHZTICKS:
			return duration;
			break;
      default:
			DP("You wanted to get a not existing TimeUnit for a STFHiPrec64BitDuration.");
			BREAKPOINT;
         return 0;
      }
   }


// Assignment      
inline const STFHiPrec64BitDuration & STFHiPrec64BitDuration::operator=(const STFLoPrec32BitDuration & duration)
   {
   this->duration = duration.Get64BitDuration(STFTU_108MHZTICKS);	
   return *this;
   }
      
inline const STFHiPrec64BitDuration & STFHiPrec64BitDuration::operator=(const STFHiPrec64BitDuration & duration)
   {
   this->duration = duration.Get64BitDuration(STFTU_108MHZTICKS);	
   return *this;
   }
      
inline const STFHiPrec64BitDuration & STFHiPrec64BitDuration::operator=(const STFHiPrec32BitDuration & duration)
   {
   this->duration = duration.Get64BitDuration(STFTU_108MHZTICKS);	
   return *this;
   }


// Increment      
inline const STFHiPrec64BitDuration & STFHiPrec64BitDuration::operator+=(const STFLoPrec32BitDuration & duration)
   {
   this->duration += duration.Get64BitDuration(STFTU_108MHZTICKS);
   return *this;
   }
      
inline const STFHiPrec64BitDuration & STFHiPrec64BitDuration::operator+=(const STFHiPrec64BitDuration & duration)
   {
   this->duration += duration.Get64BitDuration(STFTU_108MHZTICKS);
   return *this;
   }
      
inline const STFHiPrec64BitDuration & STFHiPrec64BitDuration::operator+=(const STFHiPrec32BitDuration & duration)
   {
   this->duration += duration.Get64BitDuration(STFTU_108MHZTICKS);
   return *this;
   }


// Decrement      
inline const STFHiPrec64BitDuration & STFHiPrec64BitDuration::operator-=(const STFLoPrec32BitDuration & duration)
   {
   this->duration -= duration.Get64BitDuration(STFTU_108MHZTICKS);
   return *this;
   }
      
inline const STFHiPrec64BitDuration & STFHiPrec64BitDuration::operator-=(const STFHiPrec64BitDuration & duration)
   {
   this->duration -= duration.Get64BitDuration(STFTU_108MHZTICKS);
   return *this;
   }
      
inline const STFHiPrec64BitDuration & STFHiPrec64BitDuration::operator-=(const STFHiPrec32BitDuration & duration)
   {
   this->duration -= duration.Get64BitDuration(STFTU_108MHZTICKS);
   return *this;
   }


// Difference      
inline STFHiPrec64BitDuration STFHiPrec64BitDuration::operator-(const STFLoPrec32BitDuration & duration) const
   {
   return STFHiPrec64BitDuration(this->duration - duration.Get64BitDuration(STFTU_108MHZTICKS), STFTU_108MHZTICKS);
   }
      
inline STFHiPrec64BitDuration STFHiPrec64BitDuration::operator-(const STFHiPrec64BitDuration & duration) const
   {
   return STFHiPrec64BitDuration(this->duration - duration.Get64BitDuration(STFTU_108MHZTICKS), STFTU_108MHZTICKS);
   }
      
inline STFHiPrec64BitDuration STFHiPrec64BitDuration::operator-(const STFHiPrec32BitDuration & duration) const
   {
   return STFHiPrec64BitDuration(this->duration - duration.Get64BitDuration(STFTU_108MHZTICKS), STFTU_108MHZTICKS);
   }


// Summation      
inline STFHiPrec64BitDuration STFHiPrec64BitDuration::operator+(const STFLoPrec32BitDuration & duration) const
   {
   return STFHiPrec64BitDuration(this->duration + duration.Get64BitDuration(STFTU_108MHZTICKS), STFTU_108MHZTICKS);
   }
      
inline STFHiPrec64BitDuration STFHiPrec64BitDuration::operator+(const STFHiPrec64BitDuration & duration) const
   {
   return STFHiPrec64BitDuration(this->duration + duration.Get64BitDuration(STFTU_108MHZTICKS), STFTU_108MHZTICKS);
   }
      
inline STFHiPrec64BitDuration STFHiPrec64BitDuration::operator+(const STFHiPrec32BitDuration & duration) const
   {
   return STFHiPrec64BitDuration(this->duration + duration.Get64BitDuration(STFTU_108MHZTICKS), STFTU_108MHZTICKS);
   }


// Summation with time      
inline STFHiPrec64BitTime STFHiPrec64BitDuration::operator+(const STFHiPrec64BitTime & time) const
   {
   return STFHiPrec64BitTime( this->duration + time.Get64BitTime(STFTU_108MHZTICKS), STFTU_108MHZTICKS );
   }

// Division of duration by duration, results in an integer
inline STFInt64 STFHiPrec64BitDuration::operator/ (const STFHiPrec64BitDuration & duration) const
	{
	return this->duration / duration.Get64BitDuration(STFTU_108MHZTICKS);
	}

// Equals      
inline bool STFHiPrec64BitDuration::operator==(const STFLoPrec32BitDuration & duration) const
   {
   return this->duration == duration.Get64BitDuration(STFTU_108MHZTICKS);
   }
      
inline bool STFHiPrec64BitDuration::operator==(const STFHiPrec64BitDuration & duration) const
   {
   return this->duration == duration.Get64BitDuration(STFTU_108MHZTICKS);
   }
      
inline bool STFHiPrec64BitDuration::operator==(const STFHiPrec32BitDuration & duration) const
   {
   return this->duration == duration.Get64BitDuration(STFTU_108MHZTICKS);
   }


// Equals NOT      
inline bool STFHiPrec64BitDuration::operator!=(const STFLoPrec32BitDuration & duration) const
   {
   return this->duration != duration.Get64BitDuration(STFTU_108MHZTICKS);
   }
      
inline bool STFHiPrec64BitDuration::operator!=(const STFHiPrec64BitDuration & duration) const
   {
   return this->duration != duration.Get64BitDuration(STFTU_108MHZTICKS);
   }
      
inline bool STFHiPrec64BitDuration::operator!=(const STFHiPrec32BitDuration & duration) const
   {
   return this->duration != duration.Get64BitDuration(STFTU_108MHZTICKS);
   }


// Smaller or Equal      
inline bool STFHiPrec64BitDuration::operator<=(const STFLoPrec32BitDuration & duration) const
   {
   return this->duration <= duration.Get64BitDuration(STFTU_108MHZTICKS);
   }
      
inline bool STFHiPrec64BitDuration::operator<=(const STFHiPrec64BitDuration & duration) const
   {
   return this->duration <= duration.Get64BitDuration(STFTU_108MHZTICKS);
   }
      
inline bool STFHiPrec64BitDuration::operator<=(const STFHiPrec32BitDuration & duration) const
   {
   return this->duration <= duration.Get64BitDuration(STFTU_108MHZTICKS);
   }


// Greater or Equal      
inline bool STFHiPrec64BitDuration::operator>=(const STFLoPrec32BitDuration & duration) const
   {
   return this->duration >= duration.Get64BitDuration(STFTU_108MHZTICKS);
   }
      
inline bool STFHiPrec64BitDuration::operator>=(const STFHiPrec64BitDuration & duration) const
   {
   return this->duration >= duration.Get64BitDuration(STFTU_108MHZTICKS);
   }
      
inline bool STFHiPrec64BitDuration::operator>=(const STFHiPrec32BitDuration & duration) const
   {
   return this->duration >= duration.Get64BitDuration(STFTU_108MHZTICKS);
   }


// Smaller      
inline bool STFHiPrec64BitDuration::operator<(const STFLoPrec32BitDuration & duration) const
   {
   return this->duration < duration.Get64BitDuration(STFTU_108MHZTICKS);
   }
      
inline bool STFHiPrec64BitDuration::operator<(const STFHiPrec64BitDuration & duration) const
   {
   return this->duration < duration.Get64BitDuration(STFTU_108MHZTICKS);
   }
      
inline bool STFHiPrec64BitDuration::operator<(const STFHiPrec32BitDuration & duration) const
   {
   return this->duration < duration.Get64BitDuration(STFTU_108MHZTICKS);
   }


// Greater      
inline bool STFHiPrec64BitDuration::operator>(const STFLoPrec32BitDuration & duration) const
   {
   return this->duration > duration.Get64BitDuration(STFTU_108MHZTICKS);
   }
      
inline bool STFHiPrec64BitDuration::operator>(const STFHiPrec64BitDuration & duration) const
   {
   return this->duration > duration.Get64BitDuration(STFTU_108MHZTICKS);
   }
      
inline bool STFHiPrec64BitDuration::operator>(const STFHiPrec32BitDuration & duration) const
   {
   return this->duration > duration.Get64BitDuration(STFTU_108MHZTICKS);
   }


// Multiply
inline STFHiPrec64BitDuration STFHiPrec64BitDuration::operator*(const int32 factor) const
   {
   return STFHiPrec64BitDuration(this->duration * factor, STFTU_108MHZTICKS);
   }


// Divide
inline STFHiPrec64BitDuration STFHiPrec64BitDuration::operator/(const int32 divider) const
   {
   return STFHiPrec64BitDuration(this->duration / divider, STFTU_108MHZTICKS);
   }



// Shift
inline STFHiPrec64BitDuration STFHiPrec64BitDuration::operator<<(const uint8 digits) const
   {
   return STFHiPrec64BitDuration(this->duration << digits, STFTU_108MHZTICKS);
   }

inline STFHiPrec64BitDuration STFHiPrec64BitDuration::operator>>(const uint8 digits) const
   {
   return STFHiPrec64BitDuration(this->duration >> digits, STFTU_108MHZTICKS);
   }


// Inside
inline bool STFHiPrec64BitDuration::Inside(const STFLoPrec32BitDuration & lower, const STFLoPrec32BitDuration & upper) const
	{
   return *this >= lower && *this <= upper;
   }

inline bool STFHiPrec64BitDuration::Inside(const STFHiPrec64BitDuration & lower, const STFHiPrec64BitDuration & upper) const
	{
   return *this >= lower && *this <= upper;
   }

inline bool STFHiPrec64BitDuration::Inside(const STFHiPrec32BitDuration & lower, const STFHiPrec32BitDuration & upper) const
	{
   return *this >= lower && *this <= upper;
   }


// GetFrequency
inline STFInt64 STFHiPrec64BitDuration::GetFrequency(void)
   {
   return LongTimeTicksPerSecond;	//this value has been fixed to 108Mhz
   }


// GetAbsoluteDuration
inline STFHiPrec64BitDuration STFHiPrec64BitDuration::GetAbsoluteDuration(void) const
   {
   return STFHiPrec64BitDuration((this->duration).Abs());
   }


//////////////////////////
// STFHiPrec32BitDuration
//////////////////////////

// Constructors
inline STFHiPrec32BitDuration::STFHiPrec32BitDuration(void) : duration (0)
	{
	}

inline STFHiPrec32BitDuration::STFHiPrec32BitDuration(int32 duration, STFTimeUnits units)
	{
   switch (units)
      {
      case STFTU_LOWSYSTEM:
         this->duration = duration * LongTimeTicksPerShortTick;
         break;
      case STFTU_HIGHSYSTEM:
         this->duration = ConvertClockSystemTo108(duration) * LongTimeTicksPerShortTick;
         break;
      case STFTU_MICROSECS:
         this->duration = duration * LongTimeTicksPerMicrosec;
         break;
      case STFTU_MILLISECS:
         this->duration = duration * LongTimeTicksPerMillisec;
         break;
      case STFTU_SECONDS:
         this->duration = duration * LongTimeTicksPerSecond;
         break;
		case STFTU_90KHZTICKS:
			this->duration = duration * 1200;
			break;
		case STFTU_108MHZTICKS:
			this->duration = duration; 
			break;
		default:
			DP("You did choose a not existing TimeUnit for a STFHiPrec32BitDuration.");
			BREAKPOINT;
      }
	}
		
inline STFHiPrec32BitDuration::STFHiPrec32BitDuration(const STFLoPrec32BitDuration & duration)
	{
	this->duration = duration.Get32BitDuration(STFTU_108MHZTICKS);
	}
		
inline STFHiPrec32BitDuration::STFHiPrec32BitDuration(const STFHiPrec64BitDuration & duration)
	{
	this->duration = duration.Get32BitDuration(STFTU_108MHZTICKS);
	}
		
inline STFHiPrec32BitDuration::STFHiPrec32BitDuration(const STFHiPrec32BitDuration & duration)
	{
	this->duration = duration.Get32BitDuration(STFTU_108MHZTICKS);
	}


// Get32BitDuration
inline int32 STFHiPrec32BitDuration::Get32BitDuration(STFTimeUnits units) const
	{
   switch (units)
      {
      case STFTU_LOWSYSTEM:
         return duration / LongTimeTicksPerShortTick;
         break;
      case STFTU_HIGHSYSTEM:
         return ConvertClock108ToSystem(duration);
         break;
      case STFTU_MICROSECS:
         return  duration / LongTimeTicksPerMicrosec;
         break;
      case STFTU_MILLISECS:
         return duration / LongTimeTicksPerMillisec;
         break;
      case STFTU_SECONDS:
         return duration / LongTimeTicksPerSecond;
         break;
		case STFTU_90KHZTICKS:
			return duration / 1200;
			break;
		case STFTU_108MHZTICKS:
			return duration;
			break;
      default:
			DP("You wanted to get a not existing TimeUnit for a STFHiPrec32BitDuration.");
			BREAKPOINT;
         return 0;
      }		
	}


// Get64BitDuration
inline STFInt64 STFHiPrec32BitDuration::Get64BitDuration(STFTimeUnits units) const
	{
   switch (units)
      {
      case STFTU_LOWSYSTEM:
         return STFInt64(duration / LongTimeTicksPerShortTick);
         break;
      case STFTU_HIGHSYSTEM:
         return ConvertClock108ToSystem(STFInt64(duration));
         break;
      case STFTU_MICROSECS:
         return STFInt64(duration / LongTimeTicksPerMicrosec);
         break;
      case STFTU_MILLISECS:
         return STFInt64(duration / LongTimeTicksPerMillisec);
         break;
      case STFTU_SECONDS:
         return STFInt64(duration / LongTimeTicksPerSecond);
         break;
		case STFTU_90KHZTICKS:
			return STFInt64(duration / 1200);
			break;
		case STFTU_108MHZTICKS:
			return STFInt64(duration);
			break;
      default:
			DP("You wanted to get a not existing TimeUnit for a STFHiPrec32BitDuration.");
			BREAKPOINT;
         return 0;
      }
	}


// Assignment		
inline const STFHiPrec32BitDuration & STFHiPrec32BitDuration::operator=(const STFLoPrec32BitDuration & duration)
   {
	this->duration = duration.Get32BitDuration(STFTU_108MHZTICKS);
	return *this;
	}
		
inline const STFHiPrec32BitDuration & STFHiPrec32BitDuration::operator=(const STFHiPrec64BitDuration & duration)
	{
	this->duration = duration.Get32BitDuration(STFTU_108MHZTICKS);
	return *this;
	}
		
inline const STFHiPrec32BitDuration & STFHiPrec32BitDuration::operator=(const STFHiPrec32BitDuration & duration)
	{
	this->duration = duration.Get32BitDuration(STFTU_108MHZTICKS);
	return *this;
	}


// Increment		
inline const STFHiPrec32BitDuration & STFHiPrec32BitDuration::operator+=(const STFLoPrec32BitDuration & duration)
	{
	this->duration += duration.Get32BitDuration(STFTU_108MHZTICKS);
	return *this;
	}
		
inline const STFHiPrec32BitDuration & STFHiPrec32BitDuration::operator+=(const STFHiPrec64BitDuration & duration)
	{
	this->duration += duration.Get32BitDuration(STFTU_108MHZTICKS);
	return *this;
	}
		
inline const STFHiPrec32BitDuration & STFHiPrec32BitDuration::operator+=(const STFHiPrec32BitDuration & duration)
	{
	this->duration += duration.Get32BitDuration(STFTU_108MHZTICKS);
	return *this;
	}


// Decrement		
inline const STFHiPrec32BitDuration & STFHiPrec32BitDuration::operator-=(const STFLoPrec32BitDuration & duration)
	{
	this->duration -= duration.Get32BitDuration(STFTU_108MHZTICKS);
	return *this;
	}
		
inline const STFHiPrec32BitDuration & STFHiPrec32BitDuration::operator-=(const STFHiPrec64BitDuration & duration)
	{
	this->duration -= duration.Get32BitDuration(STFTU_108MHZTICKS);
	return *this;
	}
		
inline const STFHiPrec32BitDuration & STFHiPrec32BitDuration::operator-=(const STFHiPrec32BitDuration & duration)
	{
	this->duration -= duration.Get32BitDuration(STFTU_108MHZTICKS);
	return *this;
	}


// Difference		
inline STFHiPrec32BitDuration STFHiPrec32BitDuration::operator-(const STFLoPrec32BitDuration & duration) const
	{
	return STFHiPrec32BitDuration(this->duration - duration.Get32BitDuration(STFTU_108MHZTICKS	),STFTU_108MHZTICKS);
	}
		
inline STFHiPrec32BitDuration STFHiPrec32BitDuration::operator-(const STFHiPrec64BitDuration & duration) const
	{
	return STFHiPrec32BitDuration(this->duration - duration.Get32BitDuration(STFTU_108MHZTICKS	),STFTU_108MHZTICKS);
	}
		
inline STFHiPrec32BitDuration STFHiPrec32BitDuration::operator-(const STFHiPrec32BitDuration & duration) const
	{
	return STFHiPrec32BitDuration(this->duration - duration.Get32BitDuration(STFTU_108MHZTICKS	),STFTU_108MHZTICKS);
	}


// Summation		
inline STFHiPrec32BitDuration STFHiPrec32BitDuration::operator+(const STFLoPrec32BitDuration & duration) const
	{
	return STFHiPrec32BitDuration(this->duration + duration.Get32BitDuration(STFTU_108MHZTICKS	),STFTU_108MHZTICKS);
	}
		
inline STFHiPrec32BitDuration STFHiPrec32BitDuration::operator+(const STFHiPrec64BitDuration & duration) const
	{
	return STFHiPrec32BitDuration(this->duration + duration.Get32BitDuration(STFTU_108MHZTICKS	),STFTU_108MHZTICKS);
	}
		
inline STFHiPrec32BitDuration STFHiPrec32BitDuration::operator+(const STFHiPrec32BitDuration & duration) const
	{
	return STFHiPrec32BitDuration(this->duration + duration.Get32BitDuration(STFTU_108MHZTICKS	),STFTU_108MHZTICKS);
	}


// Summation with time		
inline STFHiPrec64BitTime STFHiPrec32BitDuration::operator+(const STFHiPrec64BitTime & time) const
	{
	return STFHiPrec64BitTime(this->Get64BitDuration(STFTU_108MHZTICKS) + time.Get64BitTime(STFTU_108MHZTICKS),STFTU_108MHZTICKS);
	}


// Equals		
inline bool STFHiPrec32BitDuration::operator==(const STFLoPrec32BitDuration & duration) const
	{
	return this->duration == duration.Get32BitDuration(STFTU_108MHZTICKS);
	}
		
inline bool STFHiPrec32BitDuration::operator==(const STFHiPrec64BitDuration & duration) const
	{
	return this->duration == duration.Get32BitDuration(STFTU_108MHZTICKS);
	}
		
inline bool STFHiPrec32BitDuration::operator==(const STFHiPrec32BitDuration & duration) const
	{
	return this->duration == duration.Get32BitDuration(STFTU_108MHZTICKS);
	}


// Equals NOT		
inline bool STFHiPrec32BitDuration::operator!=(const STFLoPrec32BitDuration & duration) const
	{
	return this->duration != duration.Get32BitDuration(STFTU_108MHZTICKS);
	}
		
inline bool STFHiPrec32BitDuration::operator!=(const STFHiPrec64BitDuration & duration) const
	{
	return this->duration != duration.Get32BitDuration(STFTU_108MHZTICKS);
	}
		
inline bool STFHiPrec32BitDuration::operator!=(const STFHiPrec32BitDuration & duration) const
	{
	return this->duration != duration.Get32BitDuration(STFTU_108MHZTICKS);
	}


// Smaller or Equal		
inline bool STFHiPrec32BitDuration::operator<=(const STFLoPrec32BitDuration & duration) const
	{
	return this->duration <= duration.Get32BitDuration(STFTU_108MHZTICKS);
	}
		
inline bool STFHiPrec32BitDuration::operator<=(const STFHiPrec64BitDuration & duration) const
	{
	return this->duration <= duration.Get32BitDuration(STFTU_108MHZTICKS);
	}
		
inline bool STFHiPrec32BitDuration::operator<=(const STFHiPrec32BitDuration & duration) const
	{
	return this->duration <= duration.Get32BitDuration(STFTU_108MHZTICKS);
	}


// Greater or Equal		
inline bool STFHiPrec32BitDuration::operator>=(const STFLoPrec32BitDuration & duration) const
	{
	return this->duration >= duration.Get32BitDuration(STFTU_108MHZTICKS);
	}
		
inline bool STFHiPrec32BitDuration::operator>=(const STFHiPrec64BitDuration & duration) const
	{
	return this->duration >= duration.Get32BitDuration(STFTU_108MHZTICKS);
	}
		
inline bool STFHiPrec32BitDuration::operator>=(const STFHiPrec32BitDuration & duration) const
	{
	return this->duration >= duration.Get32BitDuration(STFTU_108MHZTICKS);
	}


// Smaller		
inline bool STFHiPrec32BitDuration::operator<(const STFLoPrec32BitDuration & duration) const
	{
	return this->duration < duration.Get32BitDuration(STFTU_108MHZTICKS);
	}
		
inline bool STFHiPrec32BitDuration::operator<(const STFHiPrec64BitDuration & duration) const
	{
	return this->duration < duration.Get32BitDuration(STFTU_108MHZTICKS);
	}
		
inline bool STFHiPrec32BitDuration::operator<(const STFHiPrec32BitDuration & duration) const
	{
	return this->duration < duration.Get32BitDuration(STFTU_108MHZTICKS);
	}


// Greater		
inline bool STFHiPrec32BitDuration::operator>(const STFLoPrec32BitDuration & duration) const
	{
	return this->duration > duration.Get32BitDuration(STFTU_108MHZTICKS);
	}
		
inline bool STFHiPrec32BitDuration::operator>(const STFHiPrec64BitDuration & duration) const
	{
	return this->duration > duration.Get32BitDuration(STFTU_108MHZTICKS);
	}
		
inline bool STFHiPrec32BitDuration::operator>(const STFHiPrec32BitDuration & duration) const
	{
	return this->duration > duration.Get32BitDuration(STFTU_108MHZTICKS);
	}


// Multiply
inline STFHiPrec32BitDuration STFHiPrec32BitDuration::operator*(const int32 factor) const
   {
   return STFHiPrec32BitDuration(this->duration * factor, STFTU_108MHZTICKS);
   }


// Divide
inline STFHiPrec32BitDuration STFHiPrec32BitDuration::operator/(const int32 divider) const
   {
   return STFHiPrec32BitDuration(this->duration / divider, STFTU_108MHZTICKS);
   }



// Shift
inline STFHiPrec32BitDuration STFHiPrec32BitDuration::operator<<(const uint8 digits) const
   {
   return STFHiPrec32BitDuration(this->duration << digits, STFTU_108MHZTICKS);
   }

inline STFHiPrec32BitDuration STFHiPrec32BitDuration::operator>>(const uint8 digits) const
   {
   return STFHiPrec32BitDuration(this->duration >> digits, STFTU_108MHZTICKS);
   }


// Inside
inline bool STFHiPrec32BitDuration::Inside(const STFLoPrec32BitDuration & lower, const STFLoPrec32BitDuration & upper) const
	{
   return *this >= lower && *this <= upper;
   }

inline bool STFHiPrec32BitDuration::Inside(const STFHiPrec64BitDuration & lower, const STFHiPrec64BitDuration & upper) const
	{
   return *this >= lower && *this <= upper;
   }

inline bool STFHiPrec32BitDuration::Inside(const STFHiPrec32BitDuration & lower, const STFHiPrec32BitDuration & upper) const
	{
   return *this >= lower && *this <= upper;
   }


// GetFrequency
inline STFInt64 STFHiPrec32BitDuration::GetFrequency(void)
	{
	return LongTimeTicksPerSecond;	//this value has been fixed to 108Mhz
	}


// GetAbsoluteDuration
inline STFHiPrec32BitDuration STFHiPrec32BitDuration::GetAbsoluteDuration(void) const
   {
   if (this->duration < 0)
      return STFHiPrec32BitDuration((this->duration)* (-1));
   else
      return STFHiPrec32BitDuration((this->duration));
   }



//////////////////////
// STFHiPrec64BitTime
//////////////////////

// Constructors
inline STFHiPrec64BitTime::STFHiPrec64BitTime(void) : time(0)
	{
	}

inline STFHiPrec64BitTime::STFHiPrec64BitTime(STFInt64 time, STFTimeUnits units)
   {
   switch (units)
      {
      case STFTU_LOWSYSTEM:
         this->time = time * LongTimeTicksPerShortTick;
         break;
      case STFTU_HIGHSYSTEM:
         this->time = ConvertClockSystemTo108(time);
         break;
      case STFTU_MICROSECS:
         this->time = time * LongTimeTicksPerMicrosec;
         break;
      case STFTU_MILLISECS:
         this->time = time * LongTimeTicksPerMillisec;
         break;
      case STFTU_SECONDS:
         this->time = time * LongTimeTicksPerSecond;
			break;
		case STFTU_90KHZTICKS:
			this->time = time * 1200;
			break;
		case STFTU_108MHZTICKS:
			this->time = time;
         break;
		default:
			DP("You did choose a not existing TimeUnit for a STFHiPrec64BitTime.");
			BREAKPOINT;
      }
   }
		
inline STFHiPrec64BitTime::STFHiPrec64BitTime(const STFHiPrec64BitTime & time)
	{
	this->time = time.time;
	}


// Get32BitTime
inline int32 STFHiPrec64BitTime::Get32BitTime(STFTimeUnits units) const
	{
   switch (units)
      {
      case STFTU_LOWSYSTEM:
         return (time / LongTimeTicksPerShortTick).ToInt32();
         break;
      case STFTU_HIGHSYSTEM:
         return (ConvertClock108ToSystem(time)).ToInt32();
         break;
      case STFTU_MICROSECS:
         return (time / LongTimeTicksPerMicrosec).ToInt32();
         break;
      case STFTU_MILLISECS:
         return (time / LongTimeTicksPerMillisec).ToInt32();
         break;
      case STFTU_SECONDS:
         return (time / LongTimeTicksPerSecond).ToInt32();
         break;
		case STFTU_90KHZTICKS:
			return (time / 1200).ToInt32();
			break;
		case STFTU_108MHZTICKS:
			return (time).ToInt32();
         break;
      default:
			DP("You wanted to get a not existing TimeUnit for a STFHiPrec64BitTime.");
			BREAKPOINT;
         return 0;
      }
   }


// Get64BitTime
inline STFInt64 STFHiPrec64BitTime::Get64BitTime(STFTimeUnits units) const
	{
   switch (units)
      {
      case STFTU_LOWSYSTEM:
         return time / LongTimeTicksPerShortTick;
         break;
      case STFTU_HIGHSYSTEM:
         return ConvertClock108ToSystem(time);
         break;
      case STFTU_MICROSECS:
         return time / LongTimeTicksPerMicrosec;
         break;
      case STFTU_MILLISECS:
         return time / LongTimeTicksPerMillisec;
         break;
      case STFTU_SECONDS:
         return time / LongTimeTicksPerSecond;
         break;
		case STFTU_90KHZTICKS:
			return time / 1200;
			break;
		case STFTU_108MHZTICKS:
			return time;
         break;
      default:
			DP("You wanted to get a not existing TimeUnit for a STFHiPrec64BitTime.");
			BREAKPOINT;
         return 0;
         break;
      }
	}


// Assignment		
inline const STFHiPrec64BitTime & STFHiPrec64BitTime::operator=(const STFHiPrec64BitTime & time)
	{
	this->time = time.time;
   return *this;
	}


// Increment		
inline const STFHiPrec64BitTime & STFHiPrec64BitTime::operator+=(const STFLoPrec32BitDuration & duration)
	{
	this->time += duration.Get64BitDuration(STFTU_108MHZTICKS);
	return *this;
	}
		
inline const STFHiPrec64BitTime & STFHiPrec64BitTime::operator+=(const STFHiPrec64BitDuration & duration)
	{
	this->time += duration.Get64BitDuration(STFTU_108MHZTICKS);
	return *this;
	}
		
inline const STFHiPrec64BitTime & STFHiPrec64BitTime::operator+=(const STFHiPrec32BitDuration & duration)
	{
	this->time += duration.Get64BitDuration(STFTU_108MHZTICKS);
	return *this;
	}


// Decrement		
inline const STFHiPrec64BitTime & STFHiPrec64BitTime::operator-=(const STFLoPrec32BitDuration & duration)
	{
	this->time -= duration.Get64BitDuration(STFTU_108MHZTICKS);
	return *this;
	}
		
inline const STFHiPrec64BitTime & STFHiPrec64BitTime::operator-=(const STFHiPrec64BitDuration & duration)
	{
	this->time -= duration.Get64BitDuration(STFTU_108MHZTICKS);
	return *this;
	}
		
inline const STFHiPrec64BitTime & STFHiPrec64BitTime::operator-=(const STFHiPrec32BitDuration & duration)
	{
	this->time -= duration.Get64BitDuration(STFTU_108MHZTICKS);
	return *this;
	}


// Summation		
inline STFHiPrec64BitTime STFHiPrec64BitTime::operator+(const STFLoPrec32BitDuration & duration) const
	{
	return STFHiPrec64BitTime(this->time + duration.Get64BitDuration(STFTU_108MHZTICKS),STFTU_108MHZTICKS);
	}
		
inline STFHiPrec64BitTime STFHiPrec64BitTime::operator+(const STFHiPrec64BitDuration & duration) const
	{
	return STFHiPrec64BitTime(this->time + duration.Get64BitDuration(STFTU_108MHZTICKS),STFTU_108MHZTICKS);
	}
		
inline STFHiPrec64BitTime STFHiPrec64BitTime::operator+(const STFHiPrec32BitDuration & duration) const
	{
	return STFHiPrec64BitTime(this->time + duration.Get64BitDuration(STFTU_108MHZTICKS),STFTU_108MHZTICKS);
	}


// Difference		
inline STFHiPrec64BitTime STFHiPrec64BitTime::operator-(const STFLoPrec32BitDuration & duration) const
	{
	return STFHiPrec64BitTime(this->time - duration.Get64BitDuration(STFTU_108MHZTICKS),STFTU_108MHZTICKS);
	}
		
inline STFHiPrec64BitTime STFHiPrec64BitTime::operator-(const STFHiPrec64BitDuration & duration) const
	{
	return STFHiPrec64BitTime(this->time - duration.Get64BitDuration(STFTU_108MHZTICKS),STFTU_108MHZTICKS);
	}
		
inline STFHiPrec64BitTime STFHiPrec64BitTime::operator-(const STFHiPrec32BitDuration & duration) const
	{
	return STFHiPrec64BitTime(this->time - duration.Get64BitDuration(STFTU_108MHZTICKS),STFTU_108MHZTICKS);
	}


// Difference with time		
inline STFHiPrec64BitDuration STFHiPrec64BitTime::operator-(const STFHiPrec64BitTime & time) const
	{
	return STFHiPrec64BitDuration(this->time - time.time,STFTU_108MHZTICKS);
	}


// Equals		
inline bool STFHiPrec64BitTime::operator==(const STFHiPrec64BitTime & time) const
	{
	return this->time == time.time;
	}


// Equals NOT		
inline bool STFHiPrec64BitTime::operator!=(const STFHiPrec64BitTime & time) const
	{
	return this->time != time.time;
	}


// Smaller or Equal		
inline bool STFHiPrec64BitTime::operator<=(const STFHiPrec64BitTime & time) const
	{
	return this->time <= time.time;
	}


// Greater or Equal		
inline bool STFHiPrec64BitTime::operator>=(const STFHiPrec64BitTime & time) const
	{
	return this->time >= time.time;
	}


// Smaller		
inline bool STFHiPrec64BitTime::operator<(const STFHiPrec64BitTime & time) const
	{
	return this->time < time.time;
	}


// Greater		
inline bool STFHiPrec64BitTime::operator>(const STFHiPrec64BitTime & time) const
	{
	return this->time > time.time;
	}


// Inside		
inline bool STFHiPrec64BitTime::Inside(const STFHiPrec64BitTime & lower, const STFHiPrec64BitTime & upper) const
	{
	return *this >= lower && *this <= upper;
	}
		
inline bool STFHiPrec64BitTime::Inside(const STFHiPrec64BitTime & start, const STFHiPrec64BitDuration & period) const
	{
	return *this >= start && *this <= start + period;
	}


// GetFrequency
inline STFInt64 STFHiPrec64BitTime::GetFrequency(void)
	{
	return LongTimeTicksPerSecond;	//this value has been fixed to 108Mhz
	}

//------------------------------------------ Date and Time -----------------------------------------------

////////////
// STFDate
////////////

inline STFDate::STFDate (void)
   {
   year = 0;
   month = 0;
   day = 0;
   }

inline STFDate::STFDate(const STFDate & src) 
	{
	this->year = src.year;
	this->month = src.month;
	this->day = src.day;
	}

inline STFDate & STFDate::operator=(const STFDate & src)
	{
	this->year = src.year;
	this->month = src.month;
	this->day = src.day;

	return *this;
	}

inline STFResult STFDate::SetDate (uint16 yr, uint8 mth, uint8 dy)
   {
   uint8 daysOfMonth = this->GetDaysInMonth(mth, yr);
   
   //checking the range
   if ((yr <= 9999) && (mth > 0) && (mth <= 12) && (dy > 0) && (dy <= daysOfMonth))
      {
      year = yr;
      month = mth;
      day = dy;
      }
   else STFRES_RAISE(STFRES_RANGE_VIOLATION);

   STFRES_RAISE_OK;
   }

inline uint16 STFDate::GetYear (void) const
   {
   return year;
   }
      
inline uint8 STFDate::GetMonth (void) const
   {
   return month;
   }
    
inline uint8 STFDate::GetDay (void) const
   {
   return day;
   }

inline uint8 STFDate::GetDaysInMonth(uint8 month, uint16 year) const
   {
   // most months have 31 days
   uint8 daysOfMonth = 31;
   
   if (month == 2) // for february a leap-year calculation must be done
      {
      daysOfMonth = 28; //usually february has 28 days
      
      if ((year % 4) == 0) //except the year is dividable by 4 ==> 29 days
         {
         if ((year % 100) == 0) //except the year is dividably by 100 ==> 28 days
            {
            if ((year % 400) == 0) //except the year is dividable by 400 ==> 29 days again
               {
               daysOfMonth = 29;
               }           
            }
         else //dividable by 4 but not by 100 
            {
            daysOfMonth = 29;
            }
         }
      }
   // months that are not february and do not have 31 days
   else if((month == 4) || (month == 6) || (month == 9) || (month == 11))
      {
      daysOfMonth = 30;
      }
   
   return daysOfMonth;
   }

inline STFDate STFDate::AddDays(int32 days) const
   {
   STFDate  newDate;
   
   uint16   newyr    = this->year;
   uint8    newmth   = this->month;
   uint8    newdy    = this->day;
   
   //negative days must be substracted from the date
   if (days < 0)
      {
      while (days != 0)
         {
         if (-days < newdy ) //days to substract are less than actual day in month
            {
            newdy += days;
            days = 0;
            }
         else //days to substract are more then actual days in month (leap in month is needed)
            {
            days += newdy;
            if (newmth != 1) //leap in month backwards when not January
               {
               newmth--;
               }
            else //leap in year backwards when January
               {
               newmth = 12; 
               newyr--;
               }
            newdy = this->GetDaysInMonth(newmth, newyr);
            }
         }
      }
   else //positive value of days
      {
      while (days != 0)
         {
         if (days <= (this->GetDaysInMonth(newmth, newyr) - newdy)) //number of days to add is less than remainig days in month
            {
            newdy += days;
            days = 0;
            }
         else //number of days to add is more the remainin days in month (leap in month is needed)
            {
            days -= (this->GetDaysInMonth(newmth, newyr) - newdy + 1);
            newdy = 1;
            if (newmth != 12) //leap in month forward when not December
               {
               newmth++;
               }
            else //leap in year forward when December
               {
               newmth = 1;
               newyr++;
               }
            }
         }
      }
   
   newDate.SetDate(newyr, newmth, newdy);
   
   return newDate;
   }

inline bool STFDate::operator>(const STFDate & date) const
   {
   bool isGreater = false;
   
   if ( (this->year >= date.year) && (this->month >= date.month) && (this->day > date.day) )
      isGreater = true;
   
   return isGreater;
   }

inline bool STFDate::operator<(const STFDate & date) const
   {
   bool isSmaller = false;
   
   if ( (this->year <= date.year) && (this->month <= date.month) && (this->day < date.day) )
      isSmaller = true;
   
   return isSmaller;
   }

inline bool STFDate::operator>=(const STFDate & date) const
   {
   bool isGreaterOrEqual = false;
   
   if ( (this->year >= date.year) && (this->month >= date.month) && (this->day >= date.day) )
      isGreaterOrEqual = true;
   
   return isGreaterOrEqual;
   }

inline bool STFDate::operator<=(const STFDate & date) const
   {
   bool isSmallerOrEqual = false;
   
   if ( (this->year <= date.year) && (this->month <= date.month) && (this->day <= date.day) )
      isSmallerOrEqual = true;
   
   return isSmallerOrEqual;
   }
      
inline bool STFDate::operator==(const STFDate & date) const
   {
   return (this->year == date.year) && (this->month == date.month) && (this->day == date.day);
   }
      
inline bool STFDate::operator!=(const STFDate & date) const
   {
   return (this->year != date.year) || (this->month != date.month) || (this->day != date.day);
   }



/////////////////
// STFTimeOfDay
/////////////////

inline STFTimeOfDay::STFTimeOfDay (void)
   {
   hour = 0;
   minute = 0;
   second = 0;
   }

inline STFTimeOfDay::STFTimeOfDay(const STFTimeOfDay & src)
	{
	this->hour = src.hour;
	this->minute = src.minute;
	this->second = src.second;
	}

inline STFTimeOfDay & STFTimeOfDay::operator=(const STFTimeOfDay & src)
	{
	this->hour = src.hour;
	this->minute = src.minute;
	this->second = src.second;

	return *this;
	}

inline STFResult STFTimeOfDay::SetTimeOfDay (uint8 hr, uint8 min, uint8 sec) 
   {
   // check range
   if ((hr <= 23) && (min <= 59) && (sec <= 59))
      {
      hour = hr;
      minute = min;
      second = sec;
      }
   else STFRES_RAISE(STFRES_RANGE_VIOLATION);

   STFRES_RAISE_OK;
   }

inline uint8 STFTimeOfDay::GetHour (void) const
   {
   return hour;
   }
      
inline uint8 STFTimeOfDay::GetMinute (void) const
   {
   return minute;
   }
      
inline uint8 STFTimeOfDay::GetSecond (void) const
   {
   return second;
   }

inline STFTimeOfDay STFTimeOfDay::AddDuration(STFLoPrec32BitDuration duration, int32 & overflow) const
   {
   STFTimeOfDay newtime;

   int32 secs, newsecs;
   int32 mins, newmins;
   int32 hrs, newhrs;

   // retrieve number of seconds to add
   secs = duration.Get32BitDuration(STFTU_SECONDS);

   //calculate number of days, hours, minutes and seconds to add
   overflow = secs / (60*60*24);
   secs = secs % (60*60*24);

   hrs = secs / (60*60);
   secs = secs % (60*60);
   
   mins = secs / 60;

   secs = secs % 60;

   //calculate new time of day
   newsecs = (secs + this->second) % 60;
   if (newsecs < secs) mins++;

   newmins = (mins + this->minute) % 60;
   if (newmins < mins) hrs++;

   newhrs = (hrs + this->hour) % 24;
   if (newhrs < hrs) overflow++;

   newtime.SetTimeOfDay(newhrs, newmins, newsecs);

   return newtime;
   }

inline STFTimeOfDay STFTimeOfDay::AddDuration(STFHiPrec32BitDuration duration, int32 & overflow) const
   {
   STFTimeOfDay newtime;

   int32 secs, newsecs;
   int32 mins, newmins;
   int32 hrs, newhrs;

   // retrieve number of seconds to add
   secs = duration.Get32BitDuration(STFTU_SECONDS);

   //calculate number of days, hours, minutes and seconds to add
   overflow = secs / (60*60*24);
   secs = secs % (60*60*24);

   hrs = secs / (60*60);
   secs = secs % (60*60);
   
   mins = secs / 60;

   secs = secs % 60;

   //calculate new time of day
   newsecs = (secs + this->second) % 60;
   if (newsecs < secs) mins++;

   newmins = (mins + this->minute) % 60;
   if (newmins < mins) hrs++;

   newhrs = (hrs + this->hour) % 24;
   if (newhrs < hrs) overflow++;

   newtime.SetTimeOfDay(newhrs, newmins, newsecs);

   return newtime;
   }

inline STFTimeOfDay STFTimeOfDay::AddDuration(STFHiPrec64BitDuration duration, int32 & overflow) const
   {
   STFTimeOfDay newtime;

   int32 secs, newsecs;
   int32 mins, newmins;
   int32 hrs, newhrs;

   // retrieve number of seconds to add
   secs = duration.Get32BitDuration(STFTU_SECONDS);

   //calculate number of days, hours, minutes and seconds to add
   overflow = secs / (60*60*24);
   secs = secs % (60*60*24);

   hrs = secs / (60*60);
   secs = secs % (60*60);
   
   mins = secs / 60;

   secs = secs % 60;

   //calculate new time of day
   newsecs = (secs + this->second) % 60;
   if (newsecs < secs) mins++;

   newmins = (mins + this->minute) % 60;
   if (newmins < mins) hrs++;

   newhrs = (hrs + this->hour) % 24;
   if (newhrs < hrs) overflow++;

   newtime.SetTimeOfDay(newhrs, newmins, newsecs);

   return newtime;
   }

inline bool STFTimeOfDay::operator>(const STFTimeOfDay & time) const
   {
   bool isGreater = false;
   
   if ( (this->hour >= time.hour) && (this->minute >= time.minute) && (this->second > time.second) )
      isGreater = true;
   
   return isGreater;
   }

inline bool STFTimeOfDay::operator<(const STFTimeOfDay & time) const
   {
   bool isSmaller = false;
   
   if ( (this->hour <= time.hour) && (this->minute <= time.minute) && (this->second < time.second) )
      isSmaller = true;
   
   return isSmaller;
   }

inline bool STFTimeOfDay::operator>=(const STFTimeOfDay & time) const
   {
   bool isGreaterOrEqual = false;
   
   if ( (this->hour >= time.hour) && (this->minute >= time.minute) && (this->second >= time.second) )
      isGreaterOrEqual = true;
   
   return isGreaterOrEqual;
   }

inline bool STFTimeOfDay::operator<=(const STFTimeOfDay & time) const
   {
   bool isSmallerOrEqual = false;
   
   if ( (this->hour <= time.hour) && (this->minute <= time.minute) && (this->second <= time.second) )
      isSmallerOrEqual = true;
   
   return isSmallerOrEqual;
   }
      
inline bool STFTimeOfDay::operator==(const STFTimeOfDay & time) const
   {
   return (this->hour == time.hour) && (this->minute == time.minute) && (this->second == time.second);
   }
      
inline bool STFTimeOfDay::operator!=(const STFTimeOfDay & time) const
   {
   return (this->hour != time.hour) || (this->minute != time.minute) || (this->second != time.second);
   }


////////////////////////////////
///	global scaling methods ///
////////////////////////////////

// methods to convert to a 108Mhz clock and the other way round

inline STFInt64 ConvertClock108ToSystem(const STFInt64 & duration)
	{
	uint32	hlower,llower, hhigher, lhigher; //32bit values returned by the MUL32x32 macro.
	uint32	lower, higher;

	bool sign = false;
	
	lower = duration.Lower();
	higher = duration.Upper();
	
	if (higher & 0x80000000) 
		{
		lower = ~lower + 1;
		higher = ~higher + (lower ? 0 : 1);
		sign = true;
		}
	
	MUL32x32(lower, ClockMultiplier_108ToSystem, hlower, llower);
	MUL32x32(higher, ClockMultiplier_108ToSystem, hhigher, lhigher);
	
	hlower += lhigher;
	
	if (hlower < lhigher) hhigher++; // Ubertrag
	
	lower = (llower >> 18) | (hlower << 14);
	higher = (hlower >> 18) | (hhigher << 14);
	
	if (sign)
		{
		lower = ~lower + 1;
		higher = ~higher + (lower ? 0 : 1);
		}

	return STFInt64(lower, higher);
	}

inline int32 ConvertClock108ToSystem(const int32 & duration)
	{
	uint32 dhigher,dlower;
	uint32 uduration = duration;

	bool sign = false;

	if (uduration & 0x80000000)
		{
		sign = true;
		uduration = uduration * (-1);
		}

	MUL32x32(uduration, ClockMultiplier_108ToSystem, dhigher, dlower);
	dlower >>= 18;
	dlower = dlower | (dhigher << 14);

	if (sign) dlower = dlower * (-1);

	return dlower;
	}

inline STFInt64 ConvertClockSystemTo108(const STFInt64 & duration)
	{
	uint32	hlower,llower, hhigher, lhigher; //32bit values returned by the MUL32x32 macro.
	uint32	lower, higher;
	bool sign = false;
	
	lower = duration.Lower();
	higher = duration.Upper();
	
	if (higher & 0x80000000) 
		{
		lower = ~lower + 1;
		higher = ~higher + (lower ? 0 : 1);
		sign = true;
		}
	
	MUL32x32(lower, ClockMultiplier_SystemTo108, hlower, llower);
	MUL32x32(higher, ClockMultiplier_SystemTo108, hhigher, lhigher);
	
	hlower += lhigher;
	
	if (hlower < lhigher) hhigher++; // Ubertrag
	
	lower = (llower >> 14) | (hlower << 18);
	higher = (hlower >> 14) | (hhigher << 18);
	
	if (sign)
		{
		lower = ~lower + 1;
		higher = ~higher + (lower ? 0 : 1);
		}

	return STFInt64(lower, higher);
	}

inline int32 ConvertClockSystemTo108(const int32 & duration)
	{
	uint32 dhigher,dlower;
	uint32 uduration = duration;

	bool sign = false;

	if (uduration & 0x80000000)
		{
		sign = true;
		uduration = uduration * (-1);
		}
	
	MUL32x32(duration, ClockMultiplier_SystemTo108, dhigher, dlower);
	
	dlower >>= 14;
	dhigher <<= 18;

	dlower = dlower + dhigher;

	if (sign) dlower = dlower * (-1);

	return dlower;
	}

#endif //STFTIME_INL_H

