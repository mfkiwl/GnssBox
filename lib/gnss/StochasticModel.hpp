#pragma ident "$Id xy. cao 2017-07-04 $"

/**
 * @file StochasticModel.hpp
 * Base class to define stochastic models, plus implementations
 * of common ones.
 */

#ifndef STOCHASTICMODEL_HPP
#define STOCHASTICMODEL_HPP

//============================================================================
//
//  Revision
//
//  2012/09/23  Add method 'getCS()' for 'PhaseAmbiguityModel' class, which 
//              will return the cycle slip flags. (shjzhang)
//
//  2014/02/18  Add the 'IonoRandomWalkModel' for the slant ionospheric
//              delays estimation. (shjzhang) 
//
//  2014/10/23  Add the processing of the interrupts for the 
//              'IonoRandomWalkModel'
//
//  2015/07/15  Add a new interface for 'Prepare', which will be more
//              efficient for preparing.
//
//  2015/10/05  Add 'getPhiMatrix()/getQMatrix()' for stochastic model
//              with dependent variables. e.g. SatClockModel, SatOrbitModel
//
//  2017/07/03 Add TropGradRandomWalkModel for tropospheric gradient
//               for multi-stations.(XY. CAO, Wuhan University)     
//
//  2017/07/03 Add ISBRandomWalkModel for inter-system bias(GAL/BDS ISB)
//               for multi-stations.(XY. CAO, Wuhan University)     
//
//  2017/07/04 Add IFCBRandomWalkModel for inter-frequency bias(GLO code IFB)
//               for multi-satellites and multi-stations.(XY. CAO, Wuhan University)     
//
//  2022/01/10 remove the gnssRinex interface for phi/q computation
//
//============================================================================


#include "CommonTime.hpp"
#include "DataStructures.hpp"

using namespace utilSpace;
using namespace timeSpace;

namespace gnssSpace
{

      /** This is a base class to define stochastic models. It computes the
       *  elements of Phi and Q matrices corresponding to a constant
       *  stochastic model.
       *
       * @sa RandomWalkModel, WhiteNoiseModel, PhaseAmbiguityModel
       *
       */
   class StochasticModel
   {
   public:

         /// Default constructor
      StochasticModel() {};


         /// Get element of the state transition matrix Phi
      virtual double getPhi()
      { return 1.0; };


         /// Get element of the process noise matrix Q
      virtual double getQ()
      { return 0.0; };


      virtual void Prepare( const CommonTime& time,
                            const SourceID& source,
                            const SatID& sat,
                            typeValueMap& tData)
      { return; };


         /// Destructor
      virtual ~StochasticModel() {};


   }; // End of class 'StochasticModel'



      /** This class compute the elements of Phi and Q matrices corresponding
       *  to a random walk stochastic model.
       *
       * @sa StochasticModel, ConstantModel, WhiteNoiseModel
       *
       * \warning RandomWalkModel objets store their internal state, so you
       * MUST NOT use the SAME object to process DIFFERENT data streams.
       *
       */
   class RandomWalkModel : public StochasticModel
   {
   public:

         /// Default constructor. By default sets a very high Qprime and both
         /// previousTime and currentTime are CommonTime::BEGINNING_OF_TIME.
      RandomWalkModel()
         : qprime(90000000000.0), 
           previousTime(CommonTime::BEGINNING_OF_TIME),
           currentTime(CommonTime::BEGINNING_OF_TIME)
       {};


         /** Common constructor
          *
          * @param qp         Process spectral density: d(variance)/d(time) or
          *                   d(sigma*sigma)/d(time).
          * @param prevTime   Value of previous epoch
          *
          * \warning Beware of units: Process spectral density units are
          * sigma*sigma/time, while other models take plain sigma as input.
          * Sigma units are usually given in meters, but time units MUST BE
          * in SECONDS.
          *
          */
      RandomWalkModel( double qp,
                  const CommonTime& prevTime=CommonTime::BEGINNING_OF_TIME,
                  const CommonTime& currentTime=CommonTime::BEGINNING_OF_TIME )
         : qprime(qp), 
           previousTime(prevTime), 
           currentTime(prevTime) 
       {};


         /** Set the value of previous epoch
          *
          * @param prevTime   Value of previous epoch
          *
          */
      virtual RandomWalkModel& setPreviousTime(const CommonTime& prevTime)
      { previousTime = prevTime; return (*this); }


         /** Set the value of current epoch
          *
          * @param currTime   Value of current epoch
          *
          */
      virtual RandomWalkModel& setCurrentTime(const CommonTime& currTime)
      { currentTime = currTime; return (*this); }


         /** Set the value of process spectral density.
          *
          * @param qp         Process spectral density: d(variance)/d(time) or
          *                   d(sigma*sigma)/d(time).
          *
          * \warning Beware of units: Process spectral density units are
          * sigma*sigma/time, while other models take plain sigma as input.
          * Sigma units are usually given in meters, but time units MUST BE
          * in SECONDS.
          *
          */
      virtual RandomWalkModel& setQprime(double qp)
      { qprime = qp; return (*this); }


         /// Get element of the process noise matrix Q
      virtual double getQ();

      virtual void Prepare( const CommonTime& epoch,
                            const SourceID& source,
                            const SatID& sat,
                            typeValueMap& tData);

         /// Destructor
      virtual ~RandomWalkModel() {};


   private:


      /// Process spectral density
      double qprime;

      /// Epoch of previous measurement
      CommonTime previousTime;

      /// Epoch of current measurement
      CommonTime currentTime;


   }; // End of class 'RandomWalkModel'



      /** This class compute the elements of Phi and Q matrices corresponding
       *  to a white noise stochastic model.
       *
       * @sa StochasticModel, ConstantModel, RandomWalkModel
       *
       */
   class WhiteNoiseModel : public StochasticModel
   {
   public:


         /** Common constructor
          *
          * @param sigma   Standard deviation (sigma) of white noise process
          *
          */
      WhiteNoiseModel( double sigma = 300000.0 )
         : variance(sigma*sigma) {};


         /// Set the value of white noise sigma
      virtual WhiteNoiseModel& setSigma(double sigma)
      { variance = sigma*sigma; return (*this); }


         /// Get element of the state transition matrix Phi
      virtual double getPhi()
      { return 0.0; };


         /// Get element of the process noise matrix Q
      virtual double getQ()
      { return variance; };


         /// Destructor
      virtual ~WhiteNoiseModel() {};


   private:


         /// White noise variance
      double variance;


   }; // End of class 'WhiteNoiseModel'



      /** This class compute the elements of Phi and Q matrices corresponding
       *  to a phase ambiguity variable: Constant stochastic model within
       *  cycle slips and white noise stochastic model when a cycle slip
       *  happens.
       *
       * @sa StochasticModel, ConstantModel, WhiteNoiseModel
       *
       * \warning By default, this class expects each satellite to have
       * 'TypeID::satArc' data inserted in the GNSS Data Structure. Such data
       * are generated by 'SatArcMarker' objects. Use 'setWatchSatArc()'
       * method to change this behaviour and use cycle slip flags directly.
       * By default, the 'TypeID' of the cycle slip flag is 'TypeID::CSFlag'.
       */
   class PhaseAmbiguityModel : public StochasticModel
   {
   public:


         /** Common constructor
          *
          * @param sigma   Standard deviation (sigma) of white noise process
          *
          */
      PhaseAmbiguityModel( double sigma = 2.0E4 )                         
         : variance(sigma*sigma), cycleSlip(false), watchSatArc(true),
           csFlagType(TypeID::CSFlag) {};


         /** This method sets the 'TypeID' of the cycle slip flag to be used.
          *
          * @param type       Type of cycle slip flag to be used.
          *
          * \warning Method 'setWatchSatArc()' must be set 'false' for this
          * method to have any effect.
          */
      virtual PhaseAmbiguityModel& setCSFlagType( const TypeID& type )
      { csFlagType = type; return (*this); };


         /// Get the 'TypeID' of the cycle slip flag being used.
      virtual TypeID getCSFlagType( void )
      { return csFlagType; };



         /// Set the value of white noise sigma
      virtual PhaseAmbiguityModel& setSigma(double sigma)
      { variance = sigma*sigma; return (*this); }


         /** Feed the object with information about occurrence of cycle slips.
          *
          * @param cs   Boolean indicating if there is a cycle slip at current
          *             epoch.
          *
          */
      virtual PhaseAmbiguityModel& setCS(bool cs)
      { cycleSlip = cs; return (*this); };


         /** Get the cycle slip flag.
          *
          * @param cs   Boolean indicating if there is a cycle slip at current
          *             epoch.
          */
      virtual bool getCS(void)
      { return cycleSlip; };


         /// Set whether satellite arc will be used instead of cycle slip flag
      virtual PhaseAmbiguityModel& setWatchSatArc(bool watchArc)
      { watchSatArc = watchArc; return (*this); };



         /// Get element of the state transition matrix Phi
      virtual double getPhi();


         /// Get element of the process noise matrix Q
      virtual double getQ();


      virtual void Prepare( const CommonTime& epoch,
                            const SourceID& source,
                            const SatID& sat,
                            typeValueMap& tData)
      {
         satTypeValueMap satData;
         satData[sat] = tData;

         checkCS(source, sat, satData); 

         return;
      }


         /// Destructor
      virtual ~PhaseAmbiguityModel() {};


   private:


         /// White noise variance
      double variance;

         /// Boolean stating if there is a cycle slip at current epoch
      bool cycleSlip;

         /// Whether satellite arcs will be watched. False by default
      bool watchSatArc;

         /// 'TypeID' of the cycle slip flag being used.
      TypeID csFlagType;

         /// Map holding information regarding every satellite
      std::map<SourceID, std::map<SatID, double> > satArcMap;


         /** This method checks if a cycle slip happened.
          *
          * @param sat        Satellite.
          * @param data       Object holding the data.
          * @param source     Object holding the source of data.
          *
          */
      virtual void checkCS( const SourceID& source,
                            const SatID& sat,
                            satTypeValueMap& data );

   }; // End of class 'PhaseAmbiguityModel'



      /** This class compute the elements of Phi and Q matrices corresponding
       *  to zenital tropospheric wet delays, modeled as a random walk
       *  stochastic model.
       *
       * This class is designed to support multiple stations simultaneously
       *
       * @sa RandomWalkModel, StochasticModel, ConstantModel, WhiteNoiseModel
       *
       */
   class TropoRandomWalkModel : public StochasticModel
   {
   public:

         /// Default constructor.
      TropoRandomWalkModel()
          : qprime(5.0e-8) 
      {};


         /** Set the value of previous epoch for a given source
          *
          * @param source     SourceID whose previous epoch will be set
          * @param prevTime   Value of previous epoch
          *
          */
      virtual TropoRandomWalkModel& setPreviousTime( const SourceID& source,
                                                     const CommonTime& prevTime )
      { tmData[source].previousTime = prevTime; return (*this); };


         /** Set the value of current epoch for a given source
          *
          * @param source     SourceID whose current epoch will be set
          * @param currTime   Value of current epoch
          *
          */
      virtual TropoRandomWalkModel& setCurrentTime( const SourceID& source,
                                                   const CommonTime& currTime )
      { tmData[source].currentTime = currTime; return (*this); };


         /** Set the value of process spectral density for ALL current sources.
          *
          * @param qp         Process spectral density: d(variance)/d(time) or
          *                   d(sigma*sigma)/d(time).
          *
          * \warning Beware of units: Process spectral density units are
          * sigma*sigma/time, while other models take plain sigma as input.
          * Sigma units are usually given in meters, but time units MUST BE
          * in SECONDS.
          *
          * \warning New sources being added for processing AFTER calling
          * method 'setQprime()' will still be processed at the default process
          * spectral density for zenital wet tropospheric delay, which is set
          * to 3e-8 m*m/s (equivalent to about 1.0 cm*cm/h).
          *
          */
      virtual TropoRandomWalkModel& setQprime(double qp);


         /** Set the value of process spectral density for a given source.
          *
          * @param source     SourceID whose process spectral density will
          *                   be set.
          * @param qp         Process spectral density: d(variance)/d(time) or
          *                   d(sigma*sigma)/d(time).
          *
          * \warning Beware of units: Process spectral density units are
          * sigma*sigma/time, while other models take plain sigma as input.
          * Sigma units are usually given in meters, but time units MUST BE
          * in SECONDS.
          *
          * \warning New sources being added for processing AFTER calling
          * method 'setQprime()' will still be processed at the default process
          * spectral density for zenital wet tropospheric delay, which is set
          * to 3e-8 m*m/s (equivalent to about 1.0 cm*cm/h).
          *
          */
      virtual TropoRandomWalkModel& setQprime( const SourceID& source,
                                               double qp )
      { qprime = qp; return (*this); };



         /** Get element of the process noise matrix Q.
          *
          * \warning The element of process noise matrix Q to be returned
          * will correspond to the last "prepared" SourceID (using "Prepare()"
          * method).
          *
          */
      virtual double getQ()
      { return variance; };


      /** This method provides the stochastic model with all the available
       *  information and takes appropriate actions.
       */
      virtual void Prepare( const CommonTime& epoch,
                            const SourceID& source,
                            const SatID& sat,
                            typeValueMap& tData);


         /// Destructor
      virtual ~TropoRandomWalkModel() {};


   private:


         /// Structure holding object data
      struct tropModelData
      {
            // Default constructor initializing the data in the structure
         tropModelData() : previousTime(CommonTime::BEGINNING_OF_TIME) {};

         CommonTime previousTime;   ///< Epoch of previous measurement
         CommonTime currentTime;    ///< Epoch of current measurement

      }; // End of struct 'tropModelData'

      double qprime;          ///< Process spectral density

         /// Map holding the information regarding each source
      std::map<SourceID, tropModelData> tmData;


         /// Field holding value of current variance
      double variance;


         /** This method computes the right variance value to be returned
          *  by method 'getQ()'.
          *
          * @param sat        Satellite.
          * @param data       Object holding the data.
          * @param source     Object holding the source of data.
          *
          */
      virtual void computeQ( const SatID& sat,
                             typeValueMap& data,
                             const SourceID& source );


   }; // End of class 'TropoRandomWalkModel'



      /** This class compute the elements of Phi and Q matrices corresponding
       *  to tropospheric gradient, modeled as a random walk stochastic model.
       *
       * This class is designed to support multiple stations simultaneously
       *
       * @sa RandomWalkModel, StochasticModel, ConstantModel, WhiteNoiseModel
       *
       */
   class TropoGradRandomWalkModel : public StochasticModel
   {
   public:

         /// Default constructor.
      TropoGradRandomWalkModel()
          :qprime(5e-10)
      {};


         /** Set the value of previous epoch for a given source
          *
          * @param source     SourceID whose previous epoch will be set
          * @param prevTime   Value of previous epoch
          *
          */
      virtual TropoGradRandomWalkModel& setPreviousTime( const SourceID& source,
                                                     const CommonTime& prevTime )
      { gmData[source].previousTime = prevTime; return (*this); };


         /** Set the value of current epoch for a given source
          *
          * @param source     SourceID whose current epoch will be set
          * @param currTime   Value of current epoch
          *
          */
      virtual TropoGradRandomWalkModel& setCurrentTime( const SourceID& source,
                                                   const CommonTime& currTime )
      { gmData[source].currentTime = currTime; return (*this); };


         /** Set the value of process spectral density for ALL current sources.
          *
          * @param qp         Process spectral density: d(variance)/d(time) or
          *                   d(sigma*sigma)/d(time).
          *
          * \warning Beware of units: Process spectral density units are
          * sigma*sigma/time, while other models take plain sigma as input.
          * Sigma units are usually given in meters, but time units MUST BE
          * in SECONDS.
          *
          * \warning New sources being added for processing AFTER calling
          * method 'setQprime()' will still be processed at the default process
          * spectral density for wet tropospheric gradient, which is set
          * to 5e-10 m*m/s.
          *
          */
      virtual TropoGradRandomWalkModel& setQprime(double qp);


         /** Set the value of process spectral density for a given source.
          *
          * @param source     SourceID whose process spectral density will
          *                   be set.
          * @param qp         Process spectral density: d(variance)/d(time) or
          *                   d(sigma*sigma)/d(time).
          *
          * \warning Beware of units: Process spectral density units are
          * sigma*sigma/time, while other models take plain sigma as input.
          * Sigma units are usually given in meters, but time units MUST BE
          * in SECONDS.
          *
          * \warning New sources being added for processing AFTER calling
          * method 'setQprime()' will still be processed at the default process
          * spectral density for wet tropospheric gradient, which is set
          * to 5e-10 m*m/s.
          *
          */
      virtual TropoGradRandomWalkModel& setQprime( const SourceID& source,
                                               double qp )
      { qprime = qp; return (*this); };



         /** Get element of the process noise matrix Q.
          *
          * \warning The element of process noise matrix Q to be returned
          * will correspond to the last "prepared" SourceID (using "Prepare()"
          * method).
          *
          */
      virtual double getQ()
      { return variance; };



         /** This method provides the stochastic model with all the available
          *  information and takes appropriate actions.
          */
      virtual void Prepare( const CommonTime& epoch,
                            const SourceID& source,
                            const SatID& sat,
                            typeValueMap& tData);


         /// Destructor
      virtual ~TropoGradRandomWalkModel() {};


   private:


         /// Structure holding object data
      struct tropGradModelData
      {
            // Default constructor initializing the data in the structure
         tropGradModelData() : previousTime(CommonTime::BEGINNING_OF_TIME) {};

         CommonTime previousTime;   ///< Epoch of previous measurement
         CommonTime currentTime;    ///< Epoch of current measurement

      }; // End of struct 'tropGradModelData'

      double qprime;          ///< Process spectral density

         /// Map holding the information regarding each source
      std::map<SourceID, tropGradModelData> gmData;


         /// Field holding value of current variance
      double variance;


         /** This method computes the right variance value to be returned
          *  by method 'getQ()'.
          *
          * @param sat        Satellite.
          * @param data       Object holding the data.
          * @param source     Object holding the source of data.
          *
          */
      virtual void computeQ( const SatID& sat,
                             typeValueMap& data,
                             const SourceID& source );


   }; // End of class 'TropoGradRandomWalkModel'



      /** This class compute the elements of Phi and Q matrices corresponding
       *  to slant ionospheric delays on L1 frequency, modeled as a random walk 
       *  stochastic model.
       *
       *  This class ONLY support single station's ionospheric delay modelling,
       *  so if you want to estimate multiple station's slant ionospheric 
       *  delays simutaneously, you MUST create new stochastic model.
       *
       *  \warning It should be noted that the ionospheric delay on different
       *           frequecy is not the same, and the ratio for the ionospheric
       *           delays on different frequency chanel is inversley proportional 
       *           to their ratio of frequencies values.
       *
       *  \warning In this class, the stochastic model is setting for the 
       *           ionospheric delays on L1 frequecy.
       *
       * @sa RandomWalkModel, StochasticModel, ConstantModel, WhiteNoiseModel
       *
       */
   class IonoRandomWalkModel : public StochasticModel
   {
   public:

         /// Default constructor.
      IonoRandomWalkModel() 
         : qprime(1.0e-3), 
           insertInterrupt(true), 
           sampling(7200.0),
           tolerance(0.5),
           initialTime(CommonTime::BEGINNING_OF_TIME)
      {};


         /** Set the value of previous epoch for a given 'sat'
          *
          * @param satellite  SatID whose previous epoch will be set
          * @param prevTime   Value of previous epoch
          *
          */
      virtual IonoRandomWalkModel& setPreviousTime( const SatID& sat,
                                                    const CommonTime& prevTime )
      { imData[sat].previousTime = prevTime; return (*this); };


         /** Set the value of current epoch for a given 'sat'
          *
          * @param satellite  SatID whose current epoch will be set
          * @param currTime   Value of current epoch
          *
          */
      virtual IonoRandomWalkModel& setCurrentTime( const SatID& sat,
                                                   const CommonTime& currTime )
      { imData[sat].currentTime = currTime; return (*this); };


         /** Set the value of process spectral density for ALL current sources.
          *
          * @param qp         Process spectral density: d(variance)/d(time) or
          *                   d(sigma*sigma)/d(time).
          *
          * \warning Beware of units: Process spectral density units are
          * sigma*sigma/time, while other models take plain sigma as input.
          * Sigma units are usually given in meters, but time units MUST BE
          * in SECONDS.
          *
          * \warning New sources being added for processing AFTER calling
          * method 'setQprime()' will still be processed at the default process
          * spectral density for slant ionospheric delay, which is set
          * to 0.0001 m*m/s ( The variation is about 0.36m*m/h).
          *
          */
      virtual IonoRandomWalkModel& setQprime(double qp);


         /** Set the value of process spectral density for a given source.
          *
          * @param source     SourceID whose process spectral density will
          *                   be set.
          * @param qp         Process spectral density: d(variance)/d(time) or
          *                   d(sigma*sigma)/d(time).
          *
          * \warning Beware of units: Process spectral density units are
          * sigma*sigma/time, while other models take plain sigma as input.
          * Sigma units are usually given in meters, but time units MUST BE
          * in SECONDS.
          *
          * \warning New sources being added for processing AFTER calling
          * method 'setQprime()' will still be processed at the default process
          * spectral density for slant ionospheric delay, which is set
          * to 0.0001 m*m/s (equivalent to about 0.36 m*m/h).
          *
          */
      virtual IonoRandomWalkModel& setQprime( const SatID& sat,
                                                 double qp )
      { qprime = qp; return (*this); };


         /** Sets epoch to start inserting interruptions.
          *
          * @param initialEpoch        Epoch to start inserting interrutions.
          */
      virtual IonoRandomWalkModel& setInitialEpoch(const CommonTime& initialEpoch)
      { initialTime = initialEpoch; return (*this); };


         /** Sets whether insert interrupt for satellite ionospheric delays.
          *
          * @param initialEpoch        Epoch to start inserting interrutions.
          */
      virtual IonoRandomWalkModel& setInsertInterrupt(bool insert)
      { insertInterrupt = insert; return (*this); };


         /** Get element of the process noise matrix Q.
          *
          * \warning The element of process noise matrix Q to be returned
          * will correspond to the last "prepared" SourceID (using "Prepare()"
          * method).
          *
          */
      virtual double getQ()
      { return variance; };



         /** This method provides the stochastic model with all the available
          *  information and takes appropriate actions.
          *
          * @param sat        Satellite.
          * @param gData      Data object holding the data.
          *
          */
      virtual void Prepare( const CommonTime& epoch,
                            const SourceID& source,
                            const SatID& sat,
                            typeValueMap& tData);

         /// Destructor
      virtual ~IonoRandomWalkModel() {};


   private:

         /// Insert interrupt or not ?
      bool insertInterrupt;

         /// Sampling interval for INTERRUPTION, in seconds
      double sampling;

         /// Tolerance, in seconds
      double tolerance;

         /// Last processed epoch
      CommonTime initialTime;


         /// Structure holding object data
      struct ionoModelData
      {
            // Default constructor initializing the data in the structure
         ionoModelData() 
              : previousTime(CommonTime::BEGINNING_OF_TIME) 
         {};

         CommonTime previousTime;   ///< Epoch of previous measurement
         CommonTime currentTime;    ///< Epoch of current measurement

      }; // End of struct 'ionoModelData'


      double qprime;

         /// Map holding the information regarding each source
      std::map<SatID, ionoModelData> imData;


         /// Field holding value of current variance
      double variance;


         /** This method computes the right variance value to be returned
          *  by method 'getQ()'.
          *
          * @param sat        Satellite.
          * @param data       Object holding the data.
          * @param source     Object holding the source of data.
          *
          */
      virtual void computeQ( const SatID& sat,
                             typeValueMap& data,
                             const SourceID& source );


   }; // End of class 'IonoRandomWalkModel'



      /** This class compute the elements of Phi and Q matrices corresponding
       *  to receiver's bias(uncalibrated hardware delay), modeled as a random 
       *  walk stochastic model.
       *
       * This class is designed to support multiple stations simultaneously
       *
       * @sa RandomWalkModel, StochasticModel, ConstantModel, WhiteNoiseModel
       *
       */
   class RecBiasRandomWalkModel : public StochasticModel
   {
   public:

      /// Default constructor.
      RecBiasRandomWalkModel() 
             : qprime(1.0e-4)
      {};


      /** Set the value of previous epoch for a given source
       *
       * @param source     SourceID whose previous epoch will be set
       * @param prevTime   Value of previous epoch
       *
       */
      virtual RecBiasRandomWalkModel& setPreviousTime( const SourceID& source,
                                                       const CommonTime& prevTime )
      { rbData[source].previousTime = prevTime; return (*this); };


         /** Set the value of current epoch for a given source
          *
          * @param source     SourceID whose current epoch will be set
          * @param currTime   Value of current epoch
          *
          */
      virtual RecBiasRandomWalkModel& setCurrentTime( const SourceID& source,
                                                      const CommonTime& currTime )
      { rbData[source].currentTime = currTime; return (*this); };


         /** Set the value of process spectral density for ALL current sources.
          *
          * @param qp         Process spectral density: d(variance)/d(time) or
          *                   d(sigma*sigma)/d(time).
          *
          * \warning Beware of units: Process spectral density units are
          * sigma*sigma/time, while other models take plain sigma as input.
          * Sigma units are usually given in meters, but time units MUST BE
          * in SECONDS.
          *
          * \warning New sources being added for processing AFTER calling
          * method 'setQprime()' will still be processed at the default process
          * spectral density for receiver bias (UHD), which is set
          * to 3e-4 m*m/s (equivalent to about 1.0 m*m/h).
          *
          */
      virtual RecBiasRandomWalkModel& setQprime(double qp);


         /** Set the value of process spectral density for a given source.
          *
          * @param source     SourceID whose process spectral density will
          *                   be set.
          * @param qp         Process spectral density: d(variance)/d(time) or
          *                   d(sigma*sigma)/d(time).
          *
          * \warning Beware of units: Process spectral density units are
          * sigma*sigma/time, while other models take plain sigma as input.
          * Sigma units are usually given in meters, but time units MUST BE
          * in SECONDS.
          *
          * \warning New sources being added for processing AFTER calling
          * method 'setQprime()' will still be processed at the default process
          * spectral density for receiver bias(UHD), which is set
          * to 3e-4 m*m/s (equivalent to about 1.0 m*m/h).
          *
          */
      virtual RecBiasRandomWalkModel& setQprime( const SourceID& source,
                                                 double qp )
      { qprime = qp; return (*this); };



         /** Get element of the process noise matrix Q.
          *
          * \warning The element of process noise matrix Q to be returned
          * will correspond to the last "prepared" SourceID (using "Prepare()"
          * method).
          *
          */
      virtual double getQ()
      { return variance; };




         /** This method provides the stochastic model with all the available
          *  information and takes appropriate actions.
          *
          * @param sat        Satellite.
          * @param gData      Data object holding the data.
          *
          */
      virtual void Prepare( const CommonTime& epoch,
                            const SourceID& source,
                            const SatID& sat,
                            typeValueMap& tData);


         /// Destructor
      virtual ~RecBiasRandomWalkModel() {};


   private:


         /// Structure holding object data
      struct recBiasModelData
      {
            // Default constructor initializing the data in the structure
         recBiasModelData() 
             : previousTime(CommonTime::BEGINNING_OF_TIME) 
         {};

         CommonTime previousTime;   ///< Epoch of previous measurement
         CommonTime currentTime;    ///< Epoch of current measurement

      }; // End of struct 'recBiasModelData'


      double qprime;          ///< Process spectral density

         /// Map holding the information regarding each source
      std::map<SourceID, recBiasModelData> rbData;


         /// Field holding value of current variance
      double variance;


         /** This method computes the right variance value to be returned
          *  by method 'getQ()'.
          *
          * @param sat        Satellite.
          * @param data       Object holding the data.
          * @param source     Object holding the source of data.
          *
          */
      virtual void computeQ( const SatID& sat,
                             typeValueMap& data,
                             const SourceID& source );


   }; // End of class 'RecBiasRandomWalkModel'



      /** This class compute the elements of Phi and Q matrices corresponding
       *  to satellite's bias(uncalibrated hardware delay), modeled as a random 
       *  walk stochastic model.
       *
       * This class is designed to support multiple satellites simultaneously and
	   * suppose satellite UPDs are the same for multi-stations. 
       *
       * @sa RandomWalkModel, StochasticModel, ConstantModel, WhiteNoiseModel
       *
       */
   class SatBiasRandomWalkModel : public StochasticModel
   {
   public:

         /// Default constructor.
      SatBiasRandomWalkModel() 
        : qprime(3e-6)
      {};


         /** Set the value of previous epoch for a given source
          *
          * @param satellite  SatID whose previous epoch will be set
          * @param prevTime   Value of previous epoch
          *
          */
      virtual SatBiasRandomWalkModel& setPreviousTime( const SatID& sat,
                                                       const CommonTime& prevTime )
      { sbData[sat].previousTime = prevTime; return (*this); };


         /** Set the value of current epoch for a given source
          *
          * @param satellite  SatID whose current epoch will be set
          * @param currTime   Value of current epoch
          *
          */
      virtual SatBiasRandomWalkModel& setCurrentTime( const SatID& sat,
                                                      const CommonTime& currTime )
      { sbData[sat].currentTime = currTime; return (*this); };


         /** Set the value of process spectral density for ALL current sources.
          *
          * @param qp         Process spectral density: d(variance)/d(time) or
          *                   d(sigma*sigma)/d(time).
          *
          * \warning Beware of units: Process spectral density units are
          * sigma*sigma/time, while other models take plain sigma as input.
          * Sigma units are usually given in meters, but time units MUST BE
          * in SECONDS.
          *
          * \warning New sources being added for processing AFTER calling
          * method 'setQprime()' will still be processed at the default process
          * spectral density for satellite bias (UHD), which is set
          * to 3e-6 m*m/s (equivalent to about 10 cm*cm/h).
          *
          */
      virtual SatBiasRandomWalkModel& setQprime(double qp);


         /** Set the value of process spectral density for a given source.
          *
          * @param source     SourceID whose process spectral density will
          *                   be set.
          * @param qp         Process spectral density: d(variance)/d(time) or
          *                   d(sigma*sigma)/d(time).
          *
          * \warning Beware of units: Process spectral density units are
          * sigma*sigma/time, while other models take plain sigma as input.
          * Sigma units are usually given in meters, but time units MUST BE
          * in SECONDS.
          *
          * \warning New sources being added for processing AFTER calling
          * method 'setQprime()' will still be processed at the default process
          * spectral density for satellite bias(UHD), which is set
          * to 3e-6 m*m/s (equivalent to about 10 cm*cm/h).
          *
          */
      virtual SatBiasRandomWalkModel& setQprime( const SatID& sat,
                                                 double qp )
      { qprime = qp; return (*this); };



         /** Get element of the process noise matrix Q.
          *
          * \warning The element of process noise matrix Q to be returned
          * will correspond to the last "prepared" SourceID (using "Prepare()"
          * method).
          *
          */
      virtual double getQ()
      { return variance; };



         /** This method provides the stochastic model with all the available
          *  information and takes appropriate actions.
          */
      virtual void Prepare( const CommonTime& epoch,
                            const SourceID& source,
                            const SatID& sat,
                            typeValueMap& tData);

         /// Destructor
      virtual ~SatBiasRandomWalkModel() {};


   private:


         /// Structure holding object data
      struct satBiasModelData
      {
            // Default constructor initializing the data in the structure
            // 3.0e-6*30=1.0e-4 ~ 0.01^2 m^2~ (0.1 cycle^2)
         satBiasModelData() 
             : previousTime(CommonTime::BEGINNING_OF_TIME) 
         {};

         CommonTime previousTime;   ///< Epoch of previous measurement
         CommonTime currentTime;    ///< Epoch of current measurement

      }; // End of struct 'satBiasModelData'


         double qprime;          ///< Process spectral density

         /// Map holding the information regarding each source
      std::map<SatID, satBiasModelData> sbData;


         /// Field holding value of current variance
      double variance;


         /** This method computes the right variance value to be returned
          *  by method 'getQ()'.
          *
          * @param sat        Satellite.
          * @param data       Object holding the data.
          * @param source     Object holding the source of data.
          *
          */
      virtual void computeQ( const SatID& sat,
                             typeValueMap& data,
                             const SourceID& source );


   }; // End of class 'SatBiasRandomWalkModel'



      /** This class compute the elements of Phi and Q matrices corresponding
       *  to inter-system bias(ISB), modeled as a random walk stochastic model.
       *
       * This class is designed to support multiple stations simultaneously
	   *
	   * \Warning: mainly designed for BDS-ISB, GAL-ISB. ONLY suit for GLO-ISB
	   * if NOT consider GLONASS FDMA.
	   *
       *
       * @sa RandomWalkModel, StochasticModel, ConstantModel, WhiteNoiseModel
       *
       */
   class ISBRandomWalkModel : public StochasticModel
   {
   public:

         /// Default constructor.
      ISBRandomWalkModel()
        :qprime(9.0e-4)
      {};


         /** Set the value of previous epoch for a given source
          *
          * @param source     SourceID whose previous epoch will be set
          * @param prevTime   Value of previous epoch
          *
          */
      virtual ISBRandomWalkModel& setPreviousTime( const SourceID& source,
                                                     const CommonTime& prevTime )
      { ISBmData[source].previousTime = prevTime; return (*this); };


         /** Set the value of current epoch for a given source
          *
          * @param source     SourceID whose current epoch will be set
          * @param currTime   Value of current epoch
          *
          */
      virtual ISBRandomWalkModel& setCurrentTime( const SourceID& source,
                                                   const CommonTime& currTime )
      { ISBmData[source].currentTime = currTime; return (*this); };


         /** Set the value of process spectral density for ALL current sources.
          *
          * @param qp         Process spectral density: d(variance)/d(time) or
          *                   d(sigma*sigma)/d(time).
          *
          * \warning Beware of units: Process spectral density units are
          * sigma*sigma/time, while other models take plain sigma as input.
          * Sigma units are usually given in meters, but time units MUST BE
          * in SECONDS.
          *
          * \warning New sources being added for processing AFTER calling
          * method 'setQprime()' will still be processed at the default process
          * spectral density for inter-system bias(ISB), which is set
          * to 1e-4 m*m/s.
          *
          */
      virtual ISBRandomWalkModel& setQprime(double qp);


         /** Set the value of process spectral density for a given source.
          *
          * @param source     SourceID whose process spectral density will
          *                   be set.
          * @param qp         Process spectral density: d(variance)/d(time) or
          *                   d(sigma*sigma)/d(time).
          *
          * \warning Beware of units: Process spectral density units are
          * sigma*sigma/time, while other models take plain sigma as input.
          * Sigma units are usually given in meters, but time units MUST BE
          * in SECONDS.
          *
          * \warning New sources being added for processing AFTER calling
          * method 'setQprime()' will still be processed at the default process
          * spectral density for inter-system bias(ISB), which is set
          * to 1e-4m*m/s.
          *
          */
      virtual ISBRandomWalkModel& setQprime( const SourceID& source,
                                               double qp )
      { qprime = qp; return (*this); };



         /** Get element of the process noise matrix Q.
          *
          * \warning The element of process noise matrix Q to be returned
          * will correspond to the last "prepared" SourceID (using "Prepare()"
          * method).
          *
          */
      virtual double getQ()
      { return variance; };



         /** This method provides the stochastic model with all the available
          *  information and takes appropriate actions.
          */
      virtual void Prepare( const CommonTime& epoch,
                            const SourceID& source,
                            const SatID& sat,
                            typeValueMap& tData);


         /// Destructor
      virtual ~ISBRandomWalkModel() {};


   private:


         /// Structure holding object data
      struct ISBData
      {
            // Default constructor initializing the data in the structure
         ISBData() 
             : previousTime(CommonTime::BEGINNING_OF_TIME) {};

         CommonTime previousTime;   ///< Epoch of previous measurement
         CommonTime currentTime;    ///< Epoch of current measurement

      }; // End of struct 'ISBData'


         double qprime;          ///< Process spectral density

         /// Map holding the information regarding each source
      std::map<SourceID, ISBData> ISBmData;


         /// Field holding value of current variance
      double variance;


         /** This method computes the right variance value to be returned
          *  by method 'getQ()'.
          *
          * @param sat        Satellite.
          * @param data       Object holding the data.
          * @param source     Object holding the source of data.
          *
          */
      virtual void computeQ( const SatID& sat,
                             typeValueMap& data,
                             const SourceID& source );



   }; // End of class 'ISBRandomWalkModel'



      /** This class compute the elements of Phi and Q matrices corresponding
       *  to inter-frequency bias(IFB), modeled as a random walk stochastic model.
       *
       * This class is designed to support multiple stations simultaneously
	   *
	   * \Warning: mainly designed for GLONASS FDMA.
	   *
       *
       * @sa RandomWalkModel, StochasticModel, ConstantModel, WhiteNoiseModel
       *
       */
   class IFCBRandomWalkModel : public StochasticModel
   {
   public:

         /// Default constructor.
      IFCBRandomWalkModel() 
        :qprime(1e-4)
      {};


         /** Set the value of previous epoch for a given source
          *
          * @param source     SourceID whose previous epoch will be set
		  * @param sat		SatID whoes previous epoch will be set
          * @param prevTime   Value of previous epoch
          *
          */
      virtual IFCBRandomWalkModel& setPreviousTime( const SourceID& source,
		  const SatID& sat,
		  const CommonTime& prevTime )
      { IFCBmData[source][sat].previousTime = prevTime; return (*this); };


         /** Set the value of current epoch for a given source
          *
          * @param source     SourceID whose current epoch will be set
		  * @param sat		SatID whoes previous epoch will be set
          * @param currTime   Value of current epoch
          *
          */
      virtual IFCBRandomWalkModel& setCurrentTime( const SourceID& source,
		  const SatID& sat,
		  const CommonTime& currTime )
      { IFCBmData[source][sat].currentTime = currTime; return (*this); };


         /** Set the value of process spectral density for ALL current sources.
          *
          * @param qp         Process spectral density: d(variance)/d(time) or
          *                   d(sigma*sigma)/d(time).
          *
          * \warning Beware of units: Process spectral density units are
          * sigma*sigma/time, while other models take plain sigma as input.
          * Sigma units are usually given in meters, but time units MUST BE
          * in SECONDS.
          *
          * \warning New sources being added for processing AFTER calling
          * method 'setQprime()' will still be processed at the default process
          * spectral density for inter-frequency bias(IFCB), which is set
          * to 1e-4 m*m/s.
          *
          */
      virtual IFCBRandomWalkModel& setQprime(double qp);


         /** Set the value of process spectral density for a given source.
          *
          * @param source     SourceID whose process spectral density will
          *                   be set.
		  * @param sat			SatID whoes process spectral density will be set.
          * @param qp         Process spectral density: d(variance)/d(time) or
          *                   d(sigma*sigma)/d(time).
          *
          * \warning Beware of units: Process spectral density units are
          * sigma*sigma/time, while other models take plain sigma as input.
          * Sigma units are usually given in meters, but time units MUST BE
          * in SECONDS.
          *
          * \warning New sources being added for processing AFTER calling
          * method 'setQprime()' will still be processed at the default process
          * spectral density for inter-frequency bias(IFCB), which is set
          * to 1e-4m*m/s.
          *
          */
      virtual IFCBRandomWalkModel& setQprime( const SourceID& source,
		  const SatID& sat,
		  double qp )
      { qprime = qp; return (*this); };



         /** Get element of the process noise matrix Q.
          *
          * \warning The element of process noise matrix Q to be returned
          * will correspond to the last "prepared" SourceID (using "Prepare()"
          * method).
          *
          */
      virtual double getQ()
      { return variance; };



         /** This method provides the stochastic model with all the available
          *  information and takes appropriate actions.
          */
      virtual void Prepare( const CommonTime& epoch,
                            const SourceID& source,
                            const SatID& sat,
                            typeValueMap& tData);


         /// Destructor
      virtual ~IFCBRandomWalkModel() {};


   private:


         /// Structure holding object data
      struct IFCBData
      {
            // Default constructor initializing the data in the structure
         IFCBData() 
             :   previousTime(CommonTime::BEGINNING_OF_TIME) {};

         CommonTime previousTime;   ///< Epoch of previous measurement
         CommonTime currentTime;    ///< Epoch of current measurement

      }; // End of struct 'IFCBData'

         double qprime;          ///< Process spectral density

         /// Map holding the information regarding each source and each satellite
	  std::map<SourceID,std::map<SatID, IFCBData>  >IFCBmData;


         /// Field holding value of current variance
      double variance;


         /** This method computes the right variance value to be returned
          *  by method 'getQ()'.
          *
          * @param sat        Satellite.
          * @param data       Object holding the data.
          * @param source     Object holding the source of data.
          *
          */
      virtual void computeQ( const SatID& sat,
                             typeValueMap& data,
                             const SourceID& source );

   }; // End of class 'IFCBRandomWalkModel'


}  // End of namespace gnssSpace

#endif // STOCHASTICMODEL_HPP
