#include <cmath>
#include <cstring>
#include <Eigen/Eigenvalues>
#include <Eigen/SVD>
#include <esn/exceptions.h>
#include <esn/network_nsli.h>
#include <network_nsli.h>

namespace ESN {

    std::shared_ptr<Network> CreateNetwork(
        const NetworkParamsNSLI & params)
    {
        return std::shared_ptr<NetworkNSLI>(new NetworkNSLI(params));
    }

    NetworkNSLI::NetworkNSLI( const NetworkParamsNSLI & params )
        : mParams( params )
        , mIn( params.inputCount )
        , mWIn( params.neuronCount, params.inputCount )
        , mWInScaling( params.inputCount )
        , mWInBias( params.inputCount )
        , mX( params.neuronCount )
        , mW( params.neuronCount, params.neuronCount )
        , mOut( params.outputCount )
        , mOutScale(params.outputCount)
        , mOutBias(params.outputCount)
        , mWOut( params.outputCount, params.neuronCount )
        , mWFB()
        , mWFBScaling()
        , mAdaptiveFilter(params.outputCount)
    {
        if ( params.inputCount <= 0 )
            throw std::invalid_argument(
                "NetworkParamsNSLI::inputCount must be not null" );
        if ( params.neuronCount <= 0 )
            throw std::invalid_argument(
                "NetworkParamsNSLI::neuronCount must be not null" );
        if ( params.outputCount <= 0 )
            throw std::invalid_argument(
                "NetworkParamsNSLI::outputCount must be not null" );
        if ( !( params.leakingRateMin > 0.0 &&
                params.leakingRateMin <= 1.0 ) )
            throw std::invalid_argument(
                "NetworkParamsNSLI::leakingRateMin must be within "
                "interval (0,1]" );
        if ( !( params.leakingRateMax > 0.0 &&
                params.leakingRateMax <= 1.0 ) )
            throw std::invalid_argument(
                "NetworkParamsNSLI::leakingRateMax must be within "
                "interval (0,1]" );
        if ( params.leakingRateMin > params.leakingRateMax )
            throw std::invalid_argument(
                "NetworkParamsNSLI::leakingRateMin must be less then or "
                "equal to NetworkParamsNSLI::leakingRateMax" );
        if ( !( params.connectivity > 0.0f &&
                params.connectivity <= 1.0f ) )
            throw std::invalid_argument(
                "NetworkParamsNSLI::connectivity must be within "
                "interval (0,1]" );

        mWIn = Eigen::MatrixXf::Random(
            params.neuronCount, params.inputCount );

        Eigen::MatrixXf randomWeights =
            ( Eigen::MatrixXf::Random( params.neuronCount,
                params.neuronCount ).array().abs()
                    <= params.connectivity ).cast< float >() *
            Eigen::MatrixXf::Random( params.neuronCount,
                params.neuronCount ).array();
        if ( params.useOrthonormalMatrix )
        {
            auto svd = randomWeights.jacobiSvd(
                Eigen::ComputeFullU | Eigen::ComputeFullV );
            mW = ( svd.matrixU() * svd.matrixV() ).sparseView();
        }
        else
        {
            float spectralRadius =
                randomWeights.eigenvalues().cwiseAbs().maxCoeff();
            mW = ( randomWeights / spectralRadius *
                params.spectralRadius ).sparseView() ;
        }

        mWInScaling = Eigen::VectorXf::Constant( params.inputCount, 1.0f );
        mWInBias = Eigen::VectorXf::Zero( params.inputCount );

        mWOut = Eigen::MatrixXf::Zero(
            params.outputCount, params.neuronCount );

        mOutScale = Eigen::VectorXf::Constant(params.outputCount, 1.0f);
        mOutBias = Eigen::VectorXf::Zero(params.outputCount);

        if (params.hasOutputFeedback)
        {
            mWFB = Eigen::MatrixXf::Random(
                params.neuronCount, params.outputCount);
            mWFBScaling = Eigen::VectorXf::Constant(
                params.outputCount, 1.0f);
        }

        mLeakingRate = ( Eigen::ArrayXf::Random( params.neuronCount ) *
            ( mParams.leakingRateMax - mParams.leakingRateMin ) +
            ( mParams.leakingRateMin + mParams.leakingRateMax ) ) / 2.0f;
        mOneMinusLeakingRate = 1.0f - mLeakingRate.array();

        mIn = Eigen::VectorXf::Zero( params.inputCount );
        mX = Eigen::VectorXf::Random( params.neuronCount );
        mOut = Eigen::VectorXf::Zero( params.outputCount );

        for (int i = 0; i < params.outputCount; ++i)
            mAdaptiveFilter[i] = std::make_shared<AdaptiveFilterRLS>(
                params.neuronCount,
                params.onlineTrainingForgettingFactor,
                params.onlineTrainingInitialCovariance);
    }

    NetworkNSLI::~NetworkNSLI()
    {
    }

    void NetworkNSLI::SetInputs( const std::vector< float > & inputs )
    {
        if ( inputs.size() != mIn.rows() )
            throw std::invalid_argument( "Wrong size of the input vector" );
        mIn = (Eigen::Map<Eigen::VectorXf>(
            const_cast<float*>(inputs.data()), inputs.size()) +
            mWInBias).cwiseProduct(mWInScaling);
    }

    void NetworkNSLI::SetInputScalings(
        const std::vector< float > & scalings )
    {
        if ( scalings.size() != mParams.inputCount )
            throw std::invalid_argument(
                "Wrong size of the scalings vector" );
        mWInScaling = Eigen::Map< Eigen::VectorXf >(
            const_cast< float * >( scalings.data() ), scalings.size() );
    }

    void NetworkNSLI::SetInputBias(
        const std::vector< float > & bias )
    {
        if ( bias.size() != mParams.inputCount )
            throw std::invalid_argument(
                "Wrong size of the scalings vector" );
        mWInBias = Eigen::Map< Eigen::VectorXf >(
            const_cast< float * >( bias.data() ), bias.size() );
    }

    void NetworkNSLI::SetOutputScale(const std::vector<float> & scale)
    {
        if (scale.size() != mParams.outputCount)
            throw std::invalid_argument(
                "Wrong size of the output scale vector");
        mOutScale = Eigen::Map<Eigen::VectorXf>(
            const_cast<float*>(scale.data()), scale.size());
    }

    void NetworkNSLI::SetOutputBias(const std::vector<float> & bias)
    {
        if (bias.size() != mParams.outputCount)
            throw std::invalid_argument(
                "Wrong size of the output bias vector");
        mOutBias = Eigen::Map<Eigen::VectorXf>(
            const_cast<float*>(bias.data()), bias.size());
    }

    void NetworkNSLI::SetFeedbackScalings(
        const std::vector< float > & scalings )
    {
        if (!mParams.hasOutputFeedback)
            throw std::logic_error(
                "Trying to set up feedback scaling for a network "
                "which doesn't have an output feedback");
        if ( scalings.size() != mParams.outputCount )
            throw std::invalid_argument(
                "Wrong size of the scalings vector" );
        mWFBScaling = Eigen::Map< Eigen::VectorXf >(
            const_cast< float * >( scalings.data() ), scalings.size() );
    }

    void NetworkNSLI::Step( float step )
    {
        if ( step <= 0.0f )
            throw std::invalid_argument(
                "Step size must be positive value" );

        auto tanh = [] ( float x ) -> float { return std::tanh( x ); };

        #define TEMP mWIn * mIn + mW * mX

        #define CALC_X(val) \
            mX = mOneMinusLeakingRate.cwiseProduct(mX) + \
                mLeakingRate.cwiseProduct(val).unaryExpr(tanh)

        #define CALC_X_WITH_FB(val) \
            if (mParams.hasOutputFeedback) \
                CALC_X(TEMP + val); \
            else \
                CALC_X(TEMP)

        if ( mParams.linearOutput )
        {
            CALC_X_WITH_FB(
                mWFB * mOut.unaryExpr(tanh).cwiseProduct(mWFBScaling));
            mOut = mWOut * mX;
        }
        else
        {
            CALC_X_WITH_FB(
                mWFB * mOut.cwiseProduct(mWFBScaling));
            mOut = ( mWOut * mX ).unaryExpr( tanh );
        }

        #undef TEMP
        #undef CALC_X
        #undef CALC_X_WITH_FB

        auto isnotfinite =
            [] (float n) -> bool { return !std::isfinite(n); };
        if (mOut.unaryExpr(isnotfinite).any())
            throw OutputIsNotFinite();
    }

    void NetworkNSLI::CaptureTransformedInput(
        std::vector< float > & input )
    {
        if ( input.size() != mParams.inputCount )
            throw std::invalid_argument(
                "Size of the vector must be equal to "
                "the number of inputs" );
        for ( int i = 0; i < mParams.inputCount; ++ i )
            input[ i ] = mIn( i );
    }

    void NetworkNSLI::CaptureActivations(
        std::vector< float > & activations )
    {
        if ( activations.size() != mParams.neuronCount )
            throw std::invalid_argument(
                "Size of the vector must be equal "
                "actual number of neurons" );

        for ( int i = 0; i < mParams.neuronCount; ++ i )
            activations[ i ] = mX( i );
    }

    void NetworkNSLI::CaptureOutput( std::vector< float > & output )
    {
        if ( output.size() != mParams.outputCount )
            throw std::invalid_argument(
                "Size of the vector must be equal "
                "actual number of outputs" );

        for ( int i = 0; i < mParams.outputCount; ++ i )
            output[i] = mOut(i) * mOutScale(i) + mOutBias(i);
    }

    void NetworkNSLI::Train(
        const std::vector< std::vector< float > > & inputs,
        const std::vector< std::vector< float > > & outputs )
    {
        if ( inputs.size() == 0 )
            throw std::invalid_argument(
                "Number of samples must be not null" );
        if ( inputs.size() != outputs.size() )
            throw std::invalid_argument(
                "Number of input and output samples must be equal" );
        const unsigned kSampleCount = inputs.size();

        Eigen::MatrixXf matX( mParams.neuronCount, kSampleCount );
        Eigen::MatrixXf matY( mParams.outputCount, kSampleCount );
        for ( int i = 0; i < kSampleCount; ++ i )
        {
            SetInputs( inputs[i] );
            Step( 0.1f );
            matX.col( i ) = mX;
            matY.col( i ) = Eigen::Map< Eigen::VectorXf >(
                const_cast< float * >( outputs[i].data() ),
                    mParams.outputCount );
        }

        Eigen::MatrixXf matXT = matX.transpose();

        mWOut = ( matY * matXT * ( matX * matXT ).inverse() );
    }

    void NetworkNSLI::TrainSingleOutputOnline(
        unsigned index, float value, bool force)
    {
        // Calculate output without bias and scaling
        float _value = (value - mOutBias(index)) / mOutScale(index);

        Eigen::VectorXf w = mWOut.row(index).transpose();
        if (!mParams.linearOutput)
            mAdaptiveFilter[index]->Train(w, std::atanh(mOut(index)),
                std::atanh(_value), mX);
        else
            mAdaptiveFilter[index]->Train(w, mOut(index), _value, mX);

        mWOut.row(index) = w.transpose();

        if (force)
            mOut(index) = _value;
    }

    void NetworkNSLI::TrainOnline(
        const std::vector<float> & output, bool forceOutput)
    {
        for (unsigned i = 0; i < mParams.outputCount; ++ i)
            TrainSingleOutputOnline(i, output[i], forceOutput);
    }

} // namespace ESN
