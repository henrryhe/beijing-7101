///
/// @file STF/Interface/Types/STFTime.h
///
/// @brief timeclasses for durations
///
/// @par OWNER:
///	STF-Team
/// @par SCOPE:
///	EXTERNAL Header File
/// 
///	
/// &copy; 2002 ST Microelectronics. All Rights Reserved.
///

#ifndef STFTIME_H
#define STFTIME_H

#include "STF/Interface/STFMemoryManagement.h"
#include "STF/Interface/Types/STFBasicTypes.h"
#include "STF/Interface/Types/STFInt64.h"
#include "STF/Interface/Types/STFResult.h"
#include "STF/Interface/STFDebug.h"

//!  Time resolutions
/*!
Each time class has a specific system dependend resolution.  It can be
converted into micro, milli or full seconds and back.  Some guarantees
are given on the system resolution, see comment of the classdefinitions
- \p STFTU_HIGHSYSTEM the system specific value for High resolution times
- \p STFTU_LOWSYSTEM low resolution time of a 108Mhz clock (formerly: the system specific value for Low resolution times)
- \p STFTU_MICROSECS Time in multiples of a micro second
- \p STFTU_MILLISECS Time in multiples of a milli second
- \p STFTU_SECONDS Time in multiples of a second
- \p STFTU_108MHZTICKS Time in multiples of a 108Mhz clock (time is emulated)
*/
enum STFTimeUnits
	{
	STFTU_HIGHSYSTEM,
	STFTU_LOWSYSTEM,
	STFTU_MICROSECS,
	STFTU_MILLISECS,
	STFTU_SECONDS,
	STFTU_90KHZTICKS,
	STFTU_108MHZTICKS
	};

static const int32 MicrosecsPerShortTimeTick = 500;
static const int32 ShortTimeTicksPerMillisec	= 2;
static const int32 ShortTimeTicksPerSecond = 2000;
static const int32 LongTimeTicksPerMicrosec = 108; 
static const int32 LongTimeTicksPerMillisec = 108000;
static const int32 LongTimeTicksPerSecond = 108000000;	//108Mhz
static const int32 LongTimeTicksPerShortTick = 54000;
static const STFInt64 LongTimeTicksPerShortOverflow = (STFInt64(1) << 32) * LongTimeTicksPerShortTick;

//! Methods to convert the times from a HighSystemTime to a 108MHZClock and the other way round.
STFInt64 ConvertClock108ToSystem(const STFInt64 & duration);
int32 ConvertClock108ToSystem(const int32 & duration);
STFInt64 ConvertClockSystemTo108(const STFInt64 & duration);
int32 ConvertClockSystemTo108(const int32 & duration);

extern uint32 ClockMultiplier_SystemTo108;
extern uint32 ClockMultiplier_108ToSystem;

class STFHiPrec64BitTime;

class STFLoPrec32BitDuration;
class STFHiPrec64BitDuration;
class STFHiPrec32BitDuration;

//! Min and Max values for duration and time classes
extern STFLoPrec32BitDuration MAX_LO_PREC_32_BIT_DURATION;
extern STFLoPrec32BitDuration MIN_LO_PREC_32_BIT_DURATION;

extern STFHiPrec32BitDuration MAX_HI_PREC_32_BIT_DURATION;
extern STFHiPrec32BitDuration MIN_HI_PREC_32_BIT_DURATION;

extern STFHiPrec64BitDuration MAX_HI_PREC_64_BIT_DURATION;
extern STFHiPrec64BitDuration MIN_HI_PREC_64_BIT_DURATION;

extern STFHiPrec64BitTime MAX_HI_PREC_64_BIT_TIME;
extern STFHiPrec64BitTime MIN_HI_PREC_64_BIT_TIME;

extern STFLoPrec32BitDuration ZERO_LO_PREC_32_BIT_DURATION;
extern STFHiPrec32BitDuration ZERO_HI_PREC_32_BIT_DURATION;
extern STFHiPrec64BitDuration ZERO_HI_PREC_64_BIT_DURATION;
extern STFHiPrec64BitTime     ZERO_HI_PREC_64_BIT_TIME;

//! Low resolution duration class
/*!
This class represents a STFLoPrec32BitDuration without vtable-pointer
This durations underlying system representation guarantees a resolution in the
range of 1ms-41us (1kHz - 24kHz) and a value of +/- 12h

  
*/
class STFLoPrec32BitDuration
	{
	
	protected:
		//! Internal representation
		int32 			duration;

	public:
		//! Empty constructor
		STFLoPrec32BitDuration(void);
		
		
		//! Main constructor
		/*!
		The duration can be given in any of the timeunits (or in the
		system value if derived from another duration).  Values that result in
		durations longer than 30 minutes are not allowed.
		\param duration The duration in the given resolution
		\param units the resolution of the given duration
		*/
		STFLoPrec32BitDuration(int32 duration, STFTimeUnits units = STFTU_MILLISECS);

		
		//! Copy constructor		
		STFLoPrec32BitDuration(const STFLoPrec32BitDuration & duration);
		
		STFLoPrec32BitDuration(const STFHiPrec64BitDuration & duration);
		
		STFLoPrec32BitDuration(const STFHiPrec32BitDuration & duration);

		
		//! The Duration as int32
		/*
		* @param units the default is STFTU_MILLISECS
		* @return the Duration as int32
		*/
		int32 Get32BitDuration(STFTimeUnits units = STFTU_MILLISECS) const;

		
		//! returns the Duration as STFIn64
		/*
		* @param units the default is STF_MILLISECS
		* @return the Duration as STFIn64
		*/
		STFInt64 Get64BitDuration(STFTimeUnits units = STFTU_MILLISECS) const;
		
		
		//! Assignment
		/*!
		If a 64 bit duration is assigned to a 32 bit duration, truncation may happen
		*/
		const STFLoPrec32BitDuration & operator=(const STFLoPrec32BitDuration & duration);
		
		const STFLoPrec32BitDuration & operator=(const STFHiPrec64BitDuration & duration);
		
		const STFLoPrec32BitDuration & operator=(const STFHiPrec32BitDuration & duration);

		
		//! Increment
		const STFLoPrec32BitDuration & operator+=(const STFLoPrec32BitDuration & duration);
		
		const STFLoPrec32BitDuration & operator+=(const STFHiPrec64BitDuration & duration);
		
		const STFLoPrec32BitDuration & operator+=(const STFHiPrec32BitDuration & duration);
		
		
		//! Decrement
		const STFLoPrec32BitDuration & operator-=(const STFLoPrec32BitDuration & duration);
		
		const STFLoPrec32BitDuration & operator-=(const STFHiPrec64BitDuration & duration);
		
		const STFLoPrec32BitDuration & operator-=(const STFHiPrec32BitDuration & duration);
		
		
		//! Difference
		/**
		* calculates the difference between this and another duration
		* With 64Bit Durations a truncation may happen
		*
		* @return a new STFLoPrec32BitDuration with the calculated Value
		*/
		STFLoPrec32BitDuration operator-(const STFLoPrec32BitDuration & duration) const;
			
		STFLoPrec32BitDuration operator-(const STFHiPrec64BitDuration & duration) const;
			
		STFLoPrec32BitDuration operator-(const STFHiPrec32BitDuration & duration) const;

		
		//! Summation
		/**
		* calculates the sum of this and another Duration
		* With 64Bit Durations truncation may happen
		*
		* @return a new STFLoPrec32BitDuration with the sum
		*/
		STFLoPrec32BitDuration operator+(const STFLoPrec32BitDuration & duration) const;
			
		STFLoPrec32BitDuration operator+(const STFHiPrec64BitDuration & duration) const;
			
		STFLoPrec32BitDuration operator+(const STFHiPrec32BitDuration & duration) const;

		
		//! Add duration to a time
		/**
		* adds this duration to a Time
		*
		* @return a new STFLoPrec32BitTime with the sum
		*/
		STFHiPrec64BitTime operator+(const STFHiPrec64BitTime & time) const;

		
		//! equal
      /*! 
      Due to the varying precisions equality is highly sensitive.
      Use it only in straight tests like x = 0.
      */
		bool operator==(const STFLoPrec32BitDuration & duration) const;
		
		bool operator==(const STFHiPrec64BitDuration & duration) const;
		
		bool operator==(const STFHiPrec32BitDuration & duration) const;

		
		//! not equal
		bool operator!=(const STFLoPrec32BitDuration & duration) const;
		
		bool operator!=(const STFHiPrec64BitDuration & duration) const;
		
		bool operator!=(const STFHiPrec32BitDuration & duration) const;

		
		//! less or equal 
		bool operator<=(const STFLoPrec32BitDuration & duration) const;
		
		bool operator<=(const STFHiPrec64BitDuration & duration) const;
		
		bool operator<=(const STFHiPrec32BitDuration & duration) const;

		
		//! greater or equal
		bool operator>=(const STFLoPrec32BitDuration & duration) const;
		
		bool operator>=(const STFHiPrec64BitDuration & duration) const;
		
		bool operator>=(const STFHiPrec32BitDuration & duration) const;

		
		//! smaller than
		bool operator<(const STFLoPrec32BitDuration & duration) const;
		
		bool operator<(const STFHiPrec64BitDuration & duration) const;
		
		bool operator<(const STFHiPrec32BitDuration & duration) const;

		
		//! greater than
		bool operator>(const STFLoPrec32BitDuration & duration) const;
		
		bool operator>(const STFHiPrec64BitDuration & duration) const;
		
		bool operator>(const STFHiPrec32BitDuration & duration) const;


      //! multiply with int32
      STFLoPrec32BitDuration operator*(const int32 factor) const;


      //! divide by int32
      STFLoPrec32BitDuration operator/(const int32 divider) const;


      //! scales the duration: (duration * to) / from
      STFLoPrec32BitDuration Scale(const int32 from, const int32 to) const;


      //! multiply with fixed 16.16 fraction
      STFLoPrec32BitDuration FractMul(const int32 fract) const;


      //! divide by fixed 16.16 fraction
      STFLoPrec32BitDuration FractDiv(const int32 fract) const;


      //! left shift operator: multiply with 2^digits
      STFLoPrec32BitDuration operator<<(const uint8 digits) const;


      //! right shift operator: divide by 2^digits
      STFLoPrec32BitDuration operator>>(const uint8 digits) const;

		
      //! Check for interval
		/*!
		Checks if a duration is inside a given interval.  This is typically of better
		use than the normal relational operators, because it causes less severe
		effects if an overrun occurs.
		*/		
		bool Inside(const STFLoPrec32BitDuration & lower, const STFLoPrec32BitDuration & upper) const;

      bool Inside(const STFHiPrec64BitDuration & lower, const STFHiPrec64BitDuration & upper) const;

      bool Inside(const STFHiPrec32BitDuration & lower, const STFHiPrec32BitDuration & upper) const;


		//! returns the frequency of the underlying systemclock
		static STFInt64 GetFrequency(void);


      //! returns a duration of the same class with the according absolute value.
      STFLoPrec32BitDuration GetAbsoluteDuration(void) const;
	};
	

//! High resolution duration
/*!
This durations underlying system representation guarantees a resolution in the
range of 5-0.5 us (corresponding to 200kHz-2Mhz systemclock) ideal 1-0.5 us
and a maximum value of ~260 years
*/
class STFHiPrec64BitDuration
   {
   
   protected:
      //! Internal representation
      STFInt64				duration;
      
   public:
      //! Empty constructor
      STFHiPrec64BitDuration(void);
      
      
      //! Main constructor
      /*!
      \param duration The duration in the given resolution
      \param units the resolution of the given duration
      */
      STFHiPrec64BitDuration(STFInt64 duration, STFTimeUnits units = STFTU_MILLISECS);

      
      //! Copy constructor
      STFHiPrec64BitDuration(const STFLoPrec32BitDuration & duration);
      
      STFHiPrec64BitDuration(const STFHiPrec64BitDuration & duration);
         
      STFHiPrec64BitDuration(const STFHiPrec32BitDuration & duration);


      //! The Duration as int32
		/*
		* @param units the default is STFTU_MILLISECS
		* @return the Duration as int32
		*/
      int32 Get32BitDuration(STFTimeUnits units = STFTU_MILLISECS) const;

      
      //! returns the Duration as STFIn64
		/*
		* @param units the default is STF_MILLISECS
		* @return the Duration as STFIn64
		*/
      STFInt64 Get64BitDuration(STFTimeUnits units = STFTU_MILLISECS) const;

      
      //! Assignment
      const STFHiPrec64BitDuration & operator=(const STFLoPrec32BitDuration & duration);
      
      const STFHiPrec64BitDuration & operator=(const STFHiPrec64BitDuration & duration);
      
      const STFHiPrec64BitDuration & operator=(const STFHiPrec32BitDuration & duration);
 
      
      //! Increment
      const STFHiPrec64BitDuration & operator+=(const STFLoPrec32BitDuration & duration);
         
      const STFHiPrec64BitDuration & operator+=(const STFHiPrec64BitDuration & duration);
         
      const STFHiPrec64BitDuration & operator+=(const STFHiPrec32BitDuration & duration);

      
      //! Decrement
      const STFHiPrec64BitDuration & operator-=(const STFLoPrec32BitDuration & duration);
         
      const STFHiPrec64BitDuration & operator-=(const STFHiPrec64BitDuration & duration);
         
      const STFHiPrec64BitDuration & operator-=(const STFHiPrec32BitDuration & duration);

      
      //! Difference
      STFHiPrec64BitDuration operator-(const STFLoPrec32BitDuration & duration) const;
         
      STFHiPrec64BitDuration operator-(const STFHiPrec64BitDuration & duration) const;
         
      STFHiPrec64BitDuration operator-(const STFHiPrec32BitDuration & duration) const;

      
      //! Summation
      STFHiPrec64BitDuration operator+(const STFLoPrec32BitDuration & duration) const;
         
      STFHiPrec64BitDuration operator+(const STFHiPrec64BitDuration & duration) const;
         
      STFHiPrec64BitDuration operator+(const STFHiPrec32BitDuration & duration) const;
         
      
      //! Add duration to a time
      STFHiPrec64BitTime operator+(const STFHiPrec64BitTime & time) const;
 
		//! Divide one duration by another, result is an integer
      STFInt64 operator/(const STFHiPrec64BitDuration & duration) const;
      
      //! equal
      /*! 
      Due to the varying precisions equality is highly sensitive.
      Use it only in straight tests like x = 0.
      */
      bool operator==(const STFLoPrec32BitDuration & duration) const;
         
      bool operator==(const STFHiPrec64BitDuration & duration) const;
         
      bool operator==(const STFHiPrec32BitDuration & duration) const;
         
      
      //! not equal
      bool operator!=(const STFLoPrec32BitDuration & duration) const;
         
      bool operator!=(const STFHiPrec64BitDuration & duration) const;
         
      bool operator!=(const STFHiPrec32BitDuration & duration) const;
         
      //! less or equal
      bool operator<=(const STFLoPrec32BitDuration & duration) const;
         
      bool operator<=(const STFHiPrec64BitDuration & duration) const;
         
      bool operator<=(const STFHiPrec32BitDuration & duration) const;
         
      
      //! greater or equal
      bool operator>=(const STFLoPrec32BitDuration & duration) const;
         
      bool operator>=(const STFHiPrec64BitDuration & duration) const;
      
      bool operator>=(const STFHiPrec32BitDuration & duration) const;
         
      
      //! smaller than
      bool operator<(const STFLoPrec32BitDuration & duration) const;
         
      bool operator<(const STFHiPrec64BitDuration & duration) const;
         
      bool operator<(const STFHiPrec32BitDuration & duration) const;
         
      
      //!greater than
      bool operator>(const STFLoPrec32BitDuration & duration) const;
         
      bool operator>(const STFHiPrec64BitDuration & duration) const;
         
      bool operator>(const STFHiPrec32BitDuration & duration) const;
      

      //! multiply with int32
      STFHiPrec64BitDuration operator*(const int32 factor) const;


      //! divide by int32
      STFHiPrec64BitDuration operator/(const int32 divider) const;


      //! scales the duration: (duration * to) / from
      STFHiPrec64BitDuration Scale(const int32 from, const int32 to) const;


      //! multiply with fixed 16.16 fraction
      STFHiPrec64BitDuration FractMul(const int32 fract) const;


      //! divide by fixed 16.16 fraction
      STFHiPrec64BitDuration FractDiv(const int32 fract) const;


      //! left shift operator: multiply with 2^digits
      STFHiPrec64BitDuration operator<<(const uint8 digits) const;


      //! right shift operator: divide by 2^digits
      STFHiPrec64BitDuration operator>>(const uint8 digits) const;
      

      //! Check for interval
		/*!
		Checks if a duration is inside a given interval.  This is typically of better
		use than the normal relational operators, because it causes less severe
		effects if an overrun occurs.
		*/		
		bool Inside(const STFLoPrec32BitDuration & lower, const STFLoPrec32BitDuration & upper) const;

      bool Inside(const STFHiPrec64BitDuration & lower, const STFHiPrec64BitDuration & upper) const;

      bool Inside(const STFHiPrec32BitDuration & lower, const STFHiPrec32BitDuration & upper) const;

      
      //! @return the frequency of the systemclock
      static STFInt64 GetFrequency(void);

      
      //! returns a duration of the same class with the according absolute value.
      STFHiPrec64BitDuration GetAbsoluteDuration(void) const;
   };
	

//! Low resolution duration class
/*!
This class represents a STFHiPrec32BitDuration without vtable-pointer
This durations underlying system representation guarantees a resolution in the
range of 0.5 - 1 us (200kHz - 2Mhz systemclock) and +/- 15min
*/
class STFHiPrec32BitDuration
	{

	protected:
		//! Internal representation
		int32 			duration;

	public:
		//! Empty constructor
		STFHiPrec32BitDuration(void);

		
		//! Main constructor
		/*!
		The duration can be given in any of the time units (or in the
		system value if derived from another duration).  Values that result in
		durations longer than 30 minutes are not allowed.
		@param duration The duration in the given resolution
		@param units the resolution of the given duration
		*/
		STFHiPrec32BitDuration(int32 duration, STFTimeUnits units = STFTU_MILLISECS);

		
		//! Copy constructor
		STFHiPrec32BitDuration(const STFLoPrec32BitDuration & duration);
		
		STFHiPrec32BitDuration(const STFHiPrec64BitDuration & duration);

		STFHiPrec32BitDuration(const STFHiPrec32BitDuration & duration);


		//! The Duration as int32
		/*
		* @param units the default is STFTU_MILLISECS
		* @return the Duration as int32
		*/
		int32 Get32BitDuration(STFTimeUnits units = STFTU_MILLISECS) const;


      //! returns the Duration as STFIn64
		/*
		* @param units the default is STF_MILLISECS
		* @return the Duration as STFIn64
		*/
		STFInt64 Get64BitDuration(STFTimeUnits units = STFTU_MILLISECS) const;

		
		//! Assignment
		/*!
		If a 64 bit duration is assigned to a 32 bit duration, truncation may happen
		*/
		const STFHiPrec32BitDuration & operator=(const STFLoPrec32BitDuration & duration);
		
		const STFHiPrec32BitDuration & operator=(const STFHiPrec64BitDuration & duration);
			
		const STFHiPrec32BitDuration & operator=(const STFHiPrec32BitDuration & duration);

		
		//! Increment
		const STFHiPrec32BitDuration & operator+=(const STFLoPrec32BitDuration & duration);
		
		const STFHiPrec32BitDuration & operator+=(const STFHiPrec64BitDuration & duration);
			
		const STFHiPrec32BitDuration & operator+=(const STFHiPrec32BitDuration & duration);

		
		//! Decrement
		const STFHiPrec32BitDuration & operator-=(const STFLoPrec32BitDuration & duration);
			
		const STFHiPrec32BitDuration & operator-=(const STFHiPrec64BitDuration & duration);
			
		const STFHiPrec32BitDuration & operator-=(const STFHiPrec32BitDuration & duration);
		
		
		//! Difference
		STFHiPrec32BitDuration operator-(const STFLoPrec32BitDuration & duration) const;
			
		STFHiPrec32BitDuration operator-(const STFHiPrec64BitDuration & duration) const;
			
		STFHiPrec32BitDuration operator-(const STFHiPrec32BitDuration & duration) const;
			
		
		//! Summation
		STFHiPrec32BitDuration operator+(const STFLoPrec32BitDuration & duration) const;
			
		STFHiPrec32BitDuration operator+(const STFHiPrec64BitDuration & duration) const;
			
		STFHiPrec32BitDuration operator+(const STFHiPrec32BitDuration & duration) const;
			
		
		//! Add duration to a time
		STFHiPrec64BitTime operator+(const STFHiPrec64BitTime & time) const;

			
		//!equals
      /*! 
      Due to the varying precisions equality is highly sensitive.
      Use it only in straight tests like x = 0.
      */
		bool operator==(const STFLoPrec32BitDuration & duration) const;
			
		bool operator==(const STFHiPrec64BitDuration & duration) const;
			
		bool operator==(const STFHiPrec32BitDuration & duration) const;
			
		
		//!not equals
		bool operator!=(const STFLoPrec32BitDuration & duration) const;
			
		bool operator!=(const STFHiPrec64BitDuration & duration) const;
			
		bool operator!=(const STFHiPrec32BitDuration & duration) const;
			
		
		//!less or equal
		bool operator<=(const STFLoPrec32BitDuration & duration) const;
			
		bool operator<=(const STFHiPrec64BitDuration & duration) const;
			
		bool operator<=(const STFHiPrec32BitDuration & duration) const;
		
		
		//!greater or equal
		bool operator>=(const STFLoPrec32BitDuration & duration) const;
			
		bool operator>=(const STFHiPrec64BitDuration & duration) const;
			
		bool operator>=(const STFHiPrec32BitDuration & duration) const;
			
		
		//!less than
		bool operator<(const STFLoPrec32BitDuration & duration) const;
			
		bool operator<(const STFHiPrec64BitDuration & duration) const;
			
		bool operator<(const STFHiPrec32BitDuration & duration) const;
			
		
		//!greater than
		bool operator>(const STFLoPrec32BitDuration & duration) const;
			
		bool operator>(const STFHiPrec64BitDuration & duration) const;
			
		bool operator>(const STFHiPrec32BitDuration & duration) const;


      //! multiply with int32
      STFHiPrec32BitDuration operator*(const int32 factor) const;


      //! divide by int32
      STFHiPrec32BitDuration operator/(const int32 divider) const;


      //! scales the duration: (duration * to) / from
      STFHiPrec32BitDuration Scale(const int32 from, const int32 to) const;


      //! multiply with fixed 16.16 fraction
      STFHiPrec32BitDuration FractMul(const int32 fract) const;


      //! divide by fixed 16.16 fraction
      STFHiPrec32BitDuration FractDiv(const int32 fract) const;


      //! left shift operator: multiply with 2^digits
      STFHiPrec32BitDuration operator<<(const uint8 digits) const;


      //! right shift operator: divide by 2^digits
      STFHiPrec32BitDuration operator>>(const uint8 digits) const;
		

      //! Check for interval
		/*!
		Checks if a duration is inside a given interval.  This is typically of better
		use than the normal relational operators, because it causes less severe
		effects if an overrun occurs.
		*/		
		bool Inside(const STFLoPrec32BitDuration & lower, const STFLoPrec32BitDuration & upper) const;

      bool Inside(const STFHiPrec64BitDuration & lower, const STFHiPrec64BitDuration & upper) const;

      bool Inside(const STFHiPrec32BitDuration & lower, const STFHiPrec32BitDuration & upper) const;
      

		//! @return the frequency of the systemclock
		static STFInt64 GetFrequency(void);


      //! returns a duration of the same class with the according absolute value.
      STFHiPrec32BitDuration GetAbsoluteDuration(void) const;
	};


	
//! High resolution system time
/*!
Time stamps of this class have a resolution in the range of 5-0.5 us preferable 1-0.5 us and a validity of
+/- 260 years.  This class should be used for long term events, that happen in the
required time horizon.  It is unlikely that this time will wrap outside its
given horizon, because it is only valid while the system is active.  It will
be reset during shutdown.  This time class has a significantly lower performance
than the short resolution version, but provides higher resolution and longer
horizon and thus less error proneness.
*/
class STFHiPrec64BitTime
	{
	
	protected:
		//! Internal time representation
		STFInt64     	time;


	public:
		//! Empty constructor
		STFHiPrec64BitTime(void);

      //! Main constructor
		/*!
		The time can be given in any of the time units (or in the
		system value if derived from another time).  This
		constructor if of limited use, because the system base time value
		is typically not known or usefull.  One would rather use the system timer
		class to retrieve the current time, and perform time arithmetic using
		durations.
		\param time The time in the given resolution
		\param units the resolution of the given time
		*/
		STFHiPrec64BitTime(STFInt64 time, STFTimeUnits units = STFTU_MILLISECS);			
	public:
		
		
		//! Copy constructor
		STFHiPrec64BitTime(const STFHiPrec64BitTime & time);
			
		
      //! The Time as uint32
		/*
		* @param units the default is STFTU_MILLISECS
		* @return the Time as uint32
		*/
		int32 Get32BitTime(STFTimeUnits units = STFTU_MILLISECS) const;

      //volatile int32 Get32BitTime(STFTimeUnits units = STFTU_MILLISECS) volatile const;
		

      //! returns the Time as STFIn64
		/*
		* @param units the default is STF_MILLISECS
		* @return the Time as STFIn64
		*/
		STFInt64 Get64BitTime(STFTimeUnits units = STFTU_MILLISECS) const;
		
		
		//! Assignment
		const STFHiPrec64BitTime & operator=(const STFHiPrec64BitTime & time);
	
		
		//! Increment
		const STFHiPrec64BitTime & operator+=(const STFLoPrec32BitDuration & duration);
			
		const STFHiPrec64BitTime & operator+=(const STFHiPrec64BitDuration & duration);
			
		const STFHiPrec64BitTime & operator+=(const STFHiPrec32BitDuration & duration);
			
		
		//! Decrement
		const STFHiPrec64BitTime & operator-=(const STFLoPrec32BitDuration & duration);
			
		const STFHiPrec64BitTime & operator-=(const STFHiPrec64BitDuration & duration);
			
		const STFHiPrec64BitTime & operator-=(const STFHiPrec32BitDuration & duration);
			
		
		//! Summation
		STFHiPrec64BitTime operator+(const STFLoPrec32BitDuration & duration) const;
			
		STFHiPrec64BitTime operator+(const STFHiPrec64BitDuration & duration) const;
			
		STFHiPrec64BitTime operator+(const STFHiPrec32BitDuration & duration) const;
			
		
		//! Subtraction
		STFHiPrec64BitTime operator-(const STFLoPrec32BitDuration & duration) const;
			
		STFHiPrec64BitTime operator-(const STFHiPrec64BitDuration & duration) const;
			
		STFHiPrec64BitTime operator-(const STFHiPrec32BitDuration & duration) const;
			
		
		//! Difference
		/*!
		Note that the difference of two time stamps is a duration
		*/
		STFHiPrec64BitDuration operator-(const STFHiPrec64BitTime & time) const;
		
		
		//! equals
      /*! 
      Due to the varying precisions equality is highly sensitive.
      Use it only in straight tests like x = 0.
      */
		bool operator==(const STFHiPrec64BitTime & time) const;
			
		
		//! not equal
		bool operator!=(const STFHiPrec64BitTime & time) const;
			

		//! Less than or equal
		bool operator<=(const STFHiPrec64BitTime & time) const;
			
		
      //! Greater than or equal
		bool operator>=(const STFHiPrec64BitTime & time) const;
			
		
		//! less than
		bool operator<(const STFHiPrec64BitTime & time) const;
			
		
		//!greater than
		bool operator>(const STFHiPrec64BitTime & time) const;
			
		
		//! Check for interval
		bool Inside(const STFHiPrec64BitTime & lower, const STFHiPrec64BitTime & upper) const;

      bool Inside(const STFHiPrec64BitTime & start, const STFHiPrec64BitDuration & period) const;

			
		//! Retrieve frequency of the systemclock.
		static STFInt64 GetFrequency(void);
		
      //! multiply with fixed 16.16 fraction
      STFHiPrec64BitTime FractMul(const int32 fract) const;
	};
	

//------------------------------------------------ Date and Time -------------------------------------------

//! A Date datatype.

class STFDate
   {
   protected:
      //! protected member variables for year, month and day
      uint16   year;
      uint8    month;
      uint8    day;

   public:
      //! Empty constructor creates a date initialized to zeros.
      STFDate (void);		

		STFDate (const STFDate & src);

      //! Set method
		/*
      * setting the date after checking the for the correct ranges, includin full leap-year check. 
      *
		* @param yr the year ranging from 0 to 9999
      * @param mth the month ranging from 1 to 12 (January to December)
      * @param dy the day ranging from 1 to 28/29/30/31 depending on the month and the year
		* @return STFRES_OK if succeded, STFRES_RANGE_VIOLATION otherwise.
		*/
      STFResult SetDate (uint16 yr, uint8 mth, uint8 dy);

      //! Get methods will return the corresponding value
      uint16 GetYear (void) const;
      uint8 GetMonth (void) const;
      uint8 GetDay (void) const;

      //! Obtaining correct number of days in a month, including full leap-year checking
      /*
      * @param month the month to check
      * @param year the year is needed only for leap-year checking
      * @return number of days in this month of the given year
      */
      uint8 GetDaysInMonth (uint8 month, uint16 year) const;

      //! Adding a number of days to this date
      /*
      * When a negative value is given as parameter it will be substracted from the date correctly
      *
      * @ param days number of days to add (can be negative)
      * @ return a new STFDate containing the calculated date (full leap-year checking)
      */
      STFDate AddDays (int32 days) const;

      //! Assignment operator
		STFDate & operator=(const STFDate & src);

      //! Greater operator
      bool operator>(const STFDate & date) const;

      //! Smaller operator
      bool operator<(const STFDate & date) const;

      //! Greater or Equal operator
      bool operator>=(const STFDate & date) const;

      //! Smaller or Equal operator
      bool operator<=(const STFDate & date) const;
      
      // Equal operator
      bool operator==(const STFDate & date) const;
      
      // Not Equal operator
      bool operator!=(const STFDate & date) const;
   };

//! A Time datatype 
//! Contains the time of day as in hr::min::sec

class STFTimeOfDay
   {
   //! protected member variables for hour, minute and second
   protected:

      uint8    hour;
      uint8    minute;
      uint8    second;

   public:

      //! Empty constructor creates a time of day initialized to zeros.
      STFTimeOfDay (void);

		STFTimeOfDay (const STFTimeOfDay & src);

      //! Set method
		/*
      * setting the time of day after checking the for the correct ranges
      *
		* @param hr the hour ranging from 0 to 23
      * @param min the minute ranging from 0 to 59
      * @param sec the second ranging from 0 to 59
		* @return STFRES_OK if succeded, STFRES_RANGE_VIOLATION otherwise.
		*/
      STFResult SetTimeOfDay (uint8 hr, uint8 min, uint8 sec);

      //! Get methods will return the corresponding value
      uint8 GetHour (void) const;
      uint8 GetMinute (void) const;
      uint8 GetSecond (void) const;

      //! Adding a duration to this time of day
      /*
      * When a negative value is given as parameter it will be substracted from the time of day correctly
      *
      * @param duration a duration to add (can be negative)
      * @param overflow a container where the number of days are written to which represent the overflow in the calculation
      * @return a new STFTimeOfDay containing the calculated time of day
      */
      STFTimeOfDay AddDuration(STFLoPrec32BitDuration duration, int32 & overflow) const;
      STFTimeOfDay AddDuration(STFHiPrec32BitDuration duration, int32 & overflow) const;
      STFTimeOfDay AddDuration(STFHiPrec64BitDuration duration, int32 & overflow) const;

      //! Assignment operator
		STFTimeOfDay & operator=(const STFTimeOfDay & src);

      //! Greater operator
      bool operator>(const STFTimeOfDay & time) const;

      //! Smaller operator
      bool operator<(const STFTimeOfDay & time) const;

      //! Greater or Equal operator
      bool operator>=(const STFTimeOfDay & time) const;

      //! Smaller or Equal operator
      bool operator<=(const STFTimeOfDay & time) const;
      
      //! Equal operator
      bool operator==(const STFTimeOfDay & time) const;
      
      //! Not Equal operator 
      bool operator!=(const STFTimeOfDay & time) const;
   };


//-------------------------------------------------- INLINES -------------------------------------------------	

#include "STF/Interface/Types/STFTime_inl.h"


#endif //STFTIME_H
