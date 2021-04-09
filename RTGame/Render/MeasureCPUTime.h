#pragma once

struct Device;
struct GPUTimeQuery;
struct CommandList;

class MeasureCPUTime
{
public:
  enum class Status
  {
    Idle,
    Started,
    Stopped,
  };

  MeasureCPUTime( Device& device );
  ~MeasureCPUTime();

  void Start( CommandList& commandList );
  void Stop( CommandList& commandList );

  void Reset();

  double GetResult( CommandList& commandList );

  Status GetStatus() const;

private:
  std::unique_ptr< GPUTimeQuery > startQuery;
  std::unique_ptr< GPUTimeQuery > stopQuery;

  bool started = false;
  bool stopped = false;
};