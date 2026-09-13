#pragma once

class G4Event;

class PrimarySource {
public:
  virtual ~PrimarySource() = default;
  virtual void Generate(G4Event *event) = 0;
  virtual int ModeId() const = 0;
};
