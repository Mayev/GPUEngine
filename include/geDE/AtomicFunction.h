#pragma once

#include<geDE/Function.h>

namespace ge{
  namespace de{
    class GEDE_EXPORT AtomicFunctionInput{
      public:
        bool                     changed     = false  ;
        Function::Ticks          updateTicks = 0      ;
        std::shared_ptr<Function>function    = nullptr;
        AtomicFunctionInput(
            std::shared_ptr<Function>const&fce         = nullptr,
            Function::Ticks                updateTicks = 0      ,
            bool                           changed     = false  );
        virtual ~AtomicFunctionInput();
    };

    class StatementFactory;

    class GEDE_EXPORT AtomicFunction: public Function{
      public:
        using SharedFunction = std::shared_ptr<Function>;
        using SharedAtomicFunctionInput = std::shared_ptr<AtomicFunctionInput>;
        using Fce2FceInput = std::map<SharedFunction,SharedAtomicFunctionInput>;
        using InputList  = std::vector<SharedAtomicFunctionInput>;
        using InputIndex = InputList::size_type;
      protected:
        InputList                _inputs               ;
        std::map<SharedFunction,std::set<InputIndex>>_fce2Indices;
        Fce2FceInput             _fce2FceInput        ;
        Function::Ticks          _updateTicks = 1      ;
        std::shared_ptr<Resource>_outputData  = nullptr;
      public:
        AtomicFunction(
            std::shared_ptr<FunctionRegister>const&fr,
            FunctionRegister::FunctionID           id);
        AtomicFunction(
            std::shared_ptr<FunctionRegister>const&fr                       ,
            TypeRegister::DescriptionVector  const&typeDescription          ,
            std::string                            name            = ""     ,
            std::shared_ptr<StatementFactory>const&factory         = nullptr);
        AtomicFunction(
            std::shared_ptr<FunctionRegister>const&fr    ,
            FunctionRegister::FunctionID           id    ,
            std::shared_ptr<Resource>        const&output);
        virtual ~AtomicFunction();
        virtual void operator()()override;
        virtual bool bindInput (
            std::shared_ptr<FunctionRegister>const&fr                ,
            InputIndex                             i                 ,
            std::shared_ptr<Function>        const&function = nullptr)override;
        virtual bool bindOutput(
            std::shared_ptr<FunctionRegister>const&fr            ,
            std::shared_ptr<Resource>        const&data = nullptr)override;
        virtual bool bindOutput(
            std::shared_ptr<FunctionRegister>const&fr                ,
            std::shared_ptr<Nullary>         const&nullary  = nullptr)override;
        virtual bool hasInput (InputIndex  i   )const override;
        virtual bool hasOutput(                )const override;
        virtual std::shared_ptr<Resource>const&getInputData (InputIndex i)const override;
        virtual std::shared_ptr<Resource>const&getOutputData(            )const override;
        virtual Ticks getUpdateTicks()const override;
        virtual void setUpdateTicks(Ticks ticks)override;
        virtual std::shared_ptr<Function>const&getInputFunction(InputIndex i)const override;
        virtual std::string doc()const override;
      protected:
        bool _processInputs();
        virtual bool _do();
        inline bool _inputChanged(InputIndex  i    )const;
    };



    inline std::shared_ptr<Resource>const&AtomicFunction::getOutputData()const{
      assert(this!=nullptr);
      return this->_outputData;
    }

    inline bool AtomicFunction::hasInput(InputIndex i)const{
      assert(this!=nullptr);
      assert(i<this->_inputs.size());
      assert(this->_inputs[i]!=nullptr);
      return this->_inputs[i]->function!=nullptr;
    }

    inline Function::Ticks AtomicFunction::getUpdateTicks()const{
      assert(this!=nullptr);
      return this->_updateTicks;
    }

    inline void AtomicFunction::setUpdateTicks(Function::Ticks ticks){
      assert(this!=nullptr);
      this->_updateTicks = ticks;
    }

    inline std::shared_ptr<Function>const&AtomicFunction::getInputFunction(InputIndex i)const{
      assert(this!=nullptr);
      assert(i<this->_inputs.size());
      assert(this->_inputs[i]!=nullptr);
      return this->_inputs[i]->function;
    }

    inline std::string AtomicFunction::doc()const{
      return"";
    }

    inline bool AtomicFunction::hasOutput()const{
      assert(this!=nullptr);
      return this->_outputData!=nullptr;
    }

    inline std::shared_ptr<Resource>const&AtomicFunction::getInputData(InputIndex i)const{
      assert(this!=nullptr);
      assert(i<this->_inputs.size());
      assert(this->_inputs[i]!=nullptr);
      assert(this->_inputs[i]->function!=nullptr);
      return this->_inputs[i]->function->getOutputData();
    }

    inline bool AtomicFunction::_inputChanged(InputIndex i)const{
      assert(this!=nullptr);
      assert(i<this->_inputs.size());
      assert(this->_inputs[i]!=nullptr);
      return this->_inputs[i]->changed;
    }

  }
}