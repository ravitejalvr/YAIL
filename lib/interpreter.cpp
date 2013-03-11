// modifications that needs to be done: add stmtlist in case expressions, it will avoid lots of repeating code.
// debug error in multiple args function call,
// add return statement.
// 
#include<iostream>
#include<cstdlib>
#include<vector>
#include<string>
#include<cstring>
#include<cstdarg>
#include<map>
#include "headers/interpreter.h"
#include "headers/string.h"
#include "headers/number.h"
#include "headers/udf.h"
#include "headers/builtins.h"
#include "headers/binaryop.h"

using namespace std;

bnk_types::Object* Interpreter::evaluate( Node* astNode, Context* execContext, int dataTypeInfo ){
    int nodeType = astNode->getType();
    switch( nodeType ){
        case __identifier:
                            bnk_astNodes::Identifier *ident;
                            ident = CAST_TO( bnk_astNodes::Identifier, astNode );
                            if( ident != NULL ){
                                string varName = ident->getName();
                                Object* rval = execContext->get(varName);
                                if( rval != NULL ){
                                    return rval;
                                }
                                else{
                                    errorMessage( 2, "Undefined variable.", ident->getName() );
                                    exit(1);
                                }
                            }
                            break;
        case __string:
                            bnk_astNodes::String *str;                                    
                            str = CAST_TO( bnk_astNodes::String, astNode );
                            if( str != NULL ){
                                return new bnk_types::String( str->getString() ); 
                            }
                            break;
        case __integer:
                            bnk_astNodes::Integer *integer;
                            integer = CAST_TO( bnk_astNodes::Integer, astNode );
                            if( integer != NULL ){
                                return new bnk_types::Integer( integer->getValue() );
                            }
                            break;
        case __nothing:
                            bnk_astNodes::Nothing *nothing;
                            nothing = CAST_TO( bnk_astNodes::Nothing, astNode );
                            if( nothing != NULL ){
                                return new bnk_types::Nothing( nothing->getValue() );
                            }
                            break;
        case __double:
                            bnk_astNodes::Double *realVal;
                            realVal = CAST_TO( bnk_astNodes::Double, astNode );
                            if( realVal != NULL ){
                                return new bnk_types::Double( realVal->getValue() );
                            }
                            break;
        case __if:
                            // evaluate the conditionalExpression node,
                            // get the truthValue, if it is true, evaluate the statements that come inside if block
                            // else evaluate either else or elif node.
                            bnk_astNodes::Operator *ifNode;
                            ifNode = CAST_TO( bnk_astNodes::Operator, astNode );
                            if( ifNode != NULL ){
                                // get the operands
                                list<Node*> *operands = ifNode->getOperands();
                                // get the cond node.
                                Object *booleanValue = this->evaluate( operands->front(), execContext, -1 );
                                bool truthValue = booleanValue->getValue()->getBooleanValue();
                                // pop the cond node.                                
                                operands->pop_front();
                                if( truthValue ){                                    
                                    // get the block node.
                                    Node *blockNode = operands->front();
                                    if( blockNode->getType() == __stmtlist ){
                                        StatementList *stmtList = CAST_TO( bnk_astNodes::StatementList, blockNode );
                                        // evaluate the statment list.
                                        int length = stmtList->getLength(), i;
                                        for( i = 0; i < length; i++ ){
                                            this->evaluate( stmtList->get(i), execContext, -1 );
                                        }
                                    }
                                    else{
                                        // evaluate single statement.
                                        this->evaluate( blockNode, execContext, -1 );
                                    }
                                }
                                else{
                                    // pop the blockNode.
                                    operands->pop_front();
                                    // evaluate the elseNode block.
                                    this->evaluate( operands->front(), execContext, -1 );
                                    operands->pop_front();
                                }
                            }
                            break;
        case __else:
                            bnk_astNodes::Operator *elseNode;
                            elseNode = CAST_TO( bnk_astNodes::Operator, astNode );
                            if( elseNode != NULL ){
                                // get the operands.
                                list<Node*> *operands = elseNode->getOperands();
                                Node *blockNode = operands->front();
                                if( blockNode->getType() == __stmtlist ){
                                    StatementList *stmtList = CAST_TO( StatementList, blockNode );
                                    // evaluate the statement list.
                                    int length = stmtList->getLength(), i;
                                    for( i = 0; i < length; i++ ){
                                        this->evaluate( stmtList->get(i), execContext, -1 );
                                    }
                                }
                                else{
                                    // evaluate single statement.
                                    this->evaluate( blockNode, execContext, -1 );
                                }
                            }
                            break;
        case __elif:
                            bnk_astNodes::Operator *elifNode;
                            elifNode = CAST_TO( bnk_astNodes::Operator, astNode );
                            bnk_astNodes::Operator *_ifNode;
                            _ifNode = new Operator( __if, elifNode->getOpsLength(), elifNode->getOperands() );
                            this->evaluate( _ifNode, execContext, -1 );
                            break;
        case __var_definition:
                                    int dataType;
                                    Operator *varDefinitionNode;
                                    varDefinitionNode = CAST_TO( Operator, astNode );
                                    if( varDefinitionNode != NULL ){
                                      // get the operands.
                                      list<Node*> *operands = varDefinitionNode->getOperands();
                                      // get the datatype of the operands.
                                      Type *dataTypeNode = CAST_TO( Type, operands->front() );
                                      if( dataTypeNode != NULL ){
                                        dataType = dataTypeNode->getDataType();
                                      }
                                      // get the variableDeclaration list.
                                      VariableList *vlist = CAST_TO( VariableList, operands->back() );
                                      if( vlist != NULL ){
                                        // go through each variable declarations and evaluate them.
                                        int length = vlist->getLength();                                        
                                        for( int i = 0; i < length; i++ ){
                                          if( !vlist->empty() ){
                                            this->evaluate( vlist->pop_front(), execContext, dataType );
                                          }
                                        }
                                      }
                                    }
                                    break;
        case __assignment:
                                Operator *assignmentNode;
                                assignmentNode = CAST_TO( Operator, astNode );
                                if( assignmentNode != NULL ){
                                  list<Node*> *operands = assignmentNode->getOperands();
                                  // getType.
                                  int dataType = dataTypeInfo;
                                  operands->pop_front();
                                  Identifier *id = CAST_TO( Identifier, operands->front() );
                                  if( id != NULL ){
                                    operands->pop_front();
                                    if( !execContext->isBound( id ) ){
                                        // get the expression node.
                                        Object *value = this->evaluate( operands->front(), execContext, dataType );
                                        // check whether the type of the expression matches
                                        // with the defined type.
                                        if( dataType == value->getDataType() ){
                                            execContext->put( string( id->getName() ), value );
                                        }
                                        else{
                                            errorMessage( 1, "Type of the expression does not match with defined type." );
                                            exit(1);
                                        }
                                    }
                                    else{
                                        errorMessage( 2, "multiple declarations of variable: ", id->getName() );
                                        exit(1);
                                    }
                                  }
                                }
                                break;
        case __funct_def:
                                Operator *functDefNode;
                                functDefNode = CAST_TO( Operator, astNode );
                                if( functDefNode != NULL ){
                                  // get the operands
                                  list<Node*> *operands = functDefNode->getOperands();
                                  Identifier  *functName = CAST_TO( Identifier, operands->front() );
                                  if( functName != NULL && !execContext->isBound( functName ) ){
                                    UserDefinedFunction *funct = new UserDefinedFunction( operands );
                                    execContext->put( functName->getName(), funct );
                                  }
                                  else{
                                      errorMessage( 2, "Multiple Declarations name: ", functName->getName() );
                                  }
                                }
                                break;
        case __funct_call:
                                Operator *functCallNode;
                                functCallNode = CAST_TO( Operator, astNode );
                                if( functCallNode != NULL ){
                                  list<Node*> *operands = functCallNode->getOperands();
                                  // get the function name.
                                  Identifier *functName = CAST_TO( Identifier, operands->front() );
                                  if( functName != NULL ){
                                    if( this->isBuiltInFunction( functName ) ){
                                      this->evaluateBuiltInFunction( functName, operands, execContext );
                                    }
                                    else if( this->isUserDefinedFunction( functName, execContext ) ){
                                      this->evaluateUserDefinedFunction( functName, operands, execContext );
                                    }
                                    else{
                                      errorMessage( 2, "Undefined function: ", functName->getName() );
                                      exit(1);
                                    }
                                  }
                                }
                                break;
        case __addition:
                                    // cast to Operator Node.
                                    Operator *addNode;
                                    addNode = CAST_TO( bnk_astNodes::Operator, astNode );
                                    return this->execOperation( addNode, execContext, new AdditionOperation() );
        case __subtraction:
                                    Operator *subNode;
                                    subNode = CAST_TO( bnk_astNodes::Operator, astNode );
                                    return this->execOperation( subNode, execContext, new SubtractionOperation() );
        case __multiplication:
                                    Operator *mulNode;
                                    mulNode = CAST_TO( bnk_astNodes::Operator, astNode );
                                    return this->execOperation( mulNode, execContext, new MultiplicationOperation() );
        case __div:
                                    Operator *divNode;
                                    divNode = CAST_TO( bnk_astNodes::Operator, astNode );
                                    return this->execOperation( divNode, execContext, new DivOperation() );
        case __power:
                                    break;
        case __or:
                                    Operator *orNode;
                                    orNode = CAST_TO( bnk_astNodes::Operator, astNode );
                                    return this->execOperation( orNode, execContext, new OrOperation() );
        case __and:
                                    Operator *andNode;
                                    andNode = CAST_TO( bnk_astNodes::Operator, astNode );
                                    return this->execOperation( andNode, execContext, new AndOperation() );
        case __lt:
                                    Operator *ltNode;
                                    ltNode = CAST_TO( bnk_astNodes::Operator, astNode );
                                    return this->execOperation( ltNode, execContext, new LessThanOperator() );
        case __gt:
                                    Operator *gtNode;
                                    gtNode = CAST_TO( bnk_astNodes::Operator, astNode );
                                    return this->execOperation( gtNode, execContext, new GreaterThanOperator() );
        case __le:
                                    Operator *leNode;
                                    leNode = CAST_TO( bnk_astNodes::Operator, astNode );
                                    return this->execOperation( leNode, execContext, new LessThanOrEqualOperator() );
        case __ge:
                                    Operator *geNode;
                                    geNode = CAST_TO( bnk_astNodes::Operator, astNode );
                                    return this->execOperation( geNode, execContext, new GreaterThanOrEqualOperator() );
        case __equality:
                                    Operator *equalityNode;
                                    equalityNode = CAST_TO( Operator, astNode );
                                    return this->execOperation( equalityNode, execContext, new EqualityOperator() );
        
    }
    return NULL;
}

Object* Interpreter::execOperation( Operator* opNode, Context* execContext, BinaryOperation* op ){
    // get the operands.
    list<Node*> *operands = opNode->getOperands();
    // get the firstOp
    Object *firstOp = this->evaluate( operands->front(), execContext, -1 );
    operands->pop_front();
    Object *secondOp = this->evaluate( operands->front(), execContext, -1 );
    operands->pop_front();
    // set the operands to the operation object.
    op->setFirstOperand( firstOp );
    op->setSecondOperand( secondOp );
    if( op->isTypeCompatible() ){
        return op->executeOperation();
    }
    else{
        errorMessage( 1, "Incompatible Operands" );
        exit(1);
    }
}

bool Interpreter::isBuiltInFunction( Identifier *functName ){
    string name = functName->getName();
    if( builtins[ name ] != NULL ){
        return true;
    }
    else{
        return false;
    }
}

bool Interpreter::isUserDefinedFunction( Identifier *functName, Context *execContext ){
    string name = functName->getName();
    Object *value = execContext->get( name );
    if( value != NULL ){
        if( this->isCallable( value ) ){
            return true;
        }
    }
    // either the value is not a function or else it does not exist.
    return false;
}

Object* Interpreter::evaluateBuiltInFunction( Identifier *functName, list<Node*> *operands, Context *execContext ){
    list<Object*> *args = new list<Object*>();
    // pop the functName.
    operands->pop_front();
    int length = operands->size();
    for( int i = 0; i < length; i++ ){
        args->push_back( this->evaluate( operands->front(), execContext, -1 ) );
        operands->pop_front();
    }
    BuiltInFunction function = getBuiltInFunction( functName );
    function(args);
}

Object* Interpreter::evaluateUserDefinedFunction( Identifier *functName, list<Node*> *arguments, Context *execContext ){
    Object *f = execContext->get( functName->getName() );
    UserDefinedFunction *function = CAST_TO( UserDefinedFunction, f );
    Context *newContext = new Context();
    // pop the function name.
    arguments->pop_front();
    
    if( function != NULL ){
        // get formal parameter list.
        FormalParameterList *fpList = function->getFormalParameterList();
        // check whether fpList and argument list length is equal or not.
        if( fpList->getLength() == arguments->size() ){
            // go through each formal parameters, check their type,
            // if everything is OK, then put it inside the new context.
            int i, length = fpList->getLength();
            for( i = 0; i < length; i++ ){
                FormalParameter *fParameter = CAST_TO( FormalParameter, fpList->get(i) );
                Object *argVal = this->evaluate( arguments->front(), execContext, -1 );
                // check fParameter and argVal type match or not.
                // if not throw error.
                if( fParameter->getDataType() == argVal->getDataType() ){
                    newContext->put( string( fParameter->getParameterName() ), argVal );
                }
                else{
                    errorMessage( 1, "Type mismatch in function call." );
                    exit(1);
                }
                arguments->pop_front();
            }
            // get the statement list and start executing the statments.
            StatementList *stmtList = function->getStatementList();
            int stLength = stmtList->getLength();
            for( i = 0; i < stLength; i++ ){
                this->evaluate( stmtList->get(i), newContext, -1 );
            }
        }
        else{
            errorMessage( 1, "argument length mismatch" );
            exit(1);
        }
    }
    return NULL;
}

void Interpreter::loadBuiltIns(void){
    builtins[ "print" ] = bnk_builtins::__bprint;
}

BuiltInFunction Interpreter::getBuiltInFunction( Identifier *functName ){
    string name = functName->getName();
    return builtins[ name ];
}

void Interpreter::errorMessage( int size, ... ){
    int i;
    char *message;
    va_list args;
    va_start( args, size );
    for( i = 0; i < size; i++ ){
        message = va_arg( args, char* );
        cout<<message;
    }
    cout<<endl;
}

bool Interpreter::isCallable( Object *value ){
    return ( value->getDataType() == __function_t );
}
