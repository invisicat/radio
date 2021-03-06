#include "workerfft.h"
#include <QVector>
#include <QFile>

WorkerFFT::WorkerFFT(QThread* thread, size_t bufferSize, size_t bufferStep) :
    m_bufferSize(bufferSize),
    m_bufferStep(bufferStep),
    m_plan(nullptr),
    m_dataSource(nullptr),
    m_safeExit(false),
    Worker(thread)
{
    if(m_bufferSize <= m_bufferStep)
    {
        qWarning("Step is bigger than buffer size; setting step to 1!");
        m_bufferStep = 1;
    }

    setObjectName(QString("FFT"));
}

void WorkerFFT::init(FFT_COMPLEX* complexSub)
{
    m_sampleBuffer = (FFT_COMPLEX*)FFT_MALLOC(sizeof(FFT_COMPLEX) * m_bufferSize);
    m_spectrumBuffer = (FFT_COMPLEX*)FFT_MALLOC(sizeof(FFT_COMPLEX) * m_bufferSize);
    m_complexSub = complexSub;

    m_plan = FFT_CREATE_PLAN(m_bufferSize, m_sampleBuffer, m_spectrumBuffer, FFTW_FORWARD, FFTW_ESTIMATE);
    m_inversePlan = FFT_CREATE_PLAN(m_bufferSize, m_spectrumBuffer, m_sampleBuffer, FFTW_BACKWARD, FFTW_ESTIMATE);
}

void WorkerFFT::setSafeExit()
{
    m_safeExit = true;
}

void WorkerFFT::work()
{
    qDebug("FFTWorker started with buffer size = %s and step = %s",
            qPrintable(QString::number(m_bufferSize)),
            qPrintable(QString::number(m_bufferStep)));

#define exit_or_wait \
    if(!m_safeExit)\
    {\
        QThread::msleep(10);\
        continue;\
    }\
    else\
    {\
        emit finished();\
        break;\
    }

    while(getActive())
    {
        if(!m_dataSource)
        {
            exit_or_wait
        }

        m_dataSource->lock();

        if(!m_dataSource->any())
        {
            m_dataSource->unlock();

            exit_or_wait
        }

        auto job = m_dataSource->dequeue();

        m_dataSource->unlock();

        if(job != nullptr)
            handleJob(job);
    }
#undef exit_or_wait
}

void WorkerFFT::setDataSource(FFTJobManager* dataSource)
{
    m_dataSource = dataSource;
}

void WorkerFFT::handleJob(FFTJob<Complex>* job)
{
    auto jobBuffer = job->getBuffer();
    size_t steps = (job->length() - m_bufferSize) / m_bufferStep + 1;
    T_REAL* buffer = new T_REAL[steps];

#ifdef DUMP_RAW
    DumpRaw(job, 0);
#endif

    for (size_t i = 0; i < steps; i++)
    {
#define welch(i,n) (1.0-((i-0.5*(n-1)) / (0.5*(n+1))) * ((i-0.5*(n-1)) / (0.5*(n+1))))
        // forward
        for (int j = 0; j < m_bufferSize; j++)
        {
            auto iter = m_sampleBuffer[j];
            Q_ASSERT(j + i * m_bufferStep < job->length());
            iter[0] = jobBuffer[j + i * m_bufferStep].real() * welch(j, m_bufferSize);
            iter[1] = jobBuffer[j + i * m_bufferStep].imag() * welch(j, m_bufferSize);
        }
        FFT_EXECUTE(m_plan);

#undef welch
        // convolute
        for(size_t j = 0; j < m_bufferSize; j++)
        {
            auto i1 = *(m_spectrumBuffer + j);
            auto i2 = *(m_complexSub + j);
            auto re = i1[0] * i2[0] - i1[1] * i2[1];
            auto im = i1[1] * i2[0] + i1[0] * i2[1];
            i1[0] = re;
            i1[1] = im;
        }

        // backward
        FFT_EXECUTE(m_inversePlan);
 /*       if (i == 0)
        {
            for(size_t j = 0; j < m_bufferSize; j++)
            {
                jobBuffer[j] = std::complex<T_REAL> (m_sampleBuffer[j][0], m_sampleBuffer[j][1]);
            }
#ifdef DUMP_RAW
            DumpRaw(job, 1);
#endif
        }*/
        auto first = *m_sampleBuffer;
        buffer[i] = log10((first[0] * first[0] + first[1] * first[1]) / m_bufferSize);
    }
    auto result = new FilterResult(buffer, steps, job->frequency());
#ifdef DUMP_FFT
    DumpFFT(result);
#endif

    emit digested(result);
}

#ifdef DUMP_FFT
void WorkerFFT::DumpFFT(FilterResult* job)
{
    QFile file(QString("dumpfft-" + QString::number(job->frequency(), 'f', 3) + "mhz.log"));
    if(!file.open(QIODevice::Append | QIODevice::WriteOnly))
        qDebug("unable to dump fft");
    else
    {
        auto b = job->getBuffer();
        QTextStream stream(&file);
        for(size_t i = 0; i < job->length(); i++)
        {
            stream << b[i];
            endl(stream);
        }
        file.close();
    }
}
#endif

#ifdef DUMP_RAW
void WorkerFFT::DumpRaw(FFTJob<Complex> *job, int Num)
{
    QFile file(QString("dumpraw" + QString::number (Num) + "-" + QString::number(job->frequency(), 'f', 3) + "mhz.log"));
    if(!file.open(QIODevice::Append | QIODevice::WriteOnly))
        qDebug("unable to dump raw samples");
    else
    {
        auto b = job->getBuffer();
        QTextStream stream(&file);
        for(size_t i = 0; i < job->length(); i++)
        {
            stream << b[i].real() << " " << b[i].imag();
            endl(stream);
        }
        file.close();
    }
}
#endif

void WorkerFFT::afterDestroy()
{
    FFT_DESTROY_PLAN(m_plan);
    FFT_DESTROY_PLAN(m_inversePlan);
    FFT_FREE(m_sampleBuffer);
    FFT_FREE(m_spectrumBuffer);
}
