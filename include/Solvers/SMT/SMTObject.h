#pragma once

class SMTFactory;

class SMTObject {
protected:
  SMTFactory *Factory;

  SMTObject(SMTFactory *F) : Factory(F) {}

  SMTObject(const SMTObject &Obj) : Factory(Obj.Factory) {}

  SMTObject &operator=(const SMTObject &Obj) {
    if (this != &Obj) {
      this->Factory = Obj.Factory;
    }
    return *this;
  }

public:
  virtual ~SMTObject() = 0;

  SMTFactory &getSMTFactory() const { return *Factory; }
};
