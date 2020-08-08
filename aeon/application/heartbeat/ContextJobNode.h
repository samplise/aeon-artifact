
class ContextJobNode: public JobManagerDataHandler{
public:
    ContextJobNode() { }

    virtual void installSignalHandlers(){ }
    virtual void start(){
        tcp = &( TcpTransport_namespace::new_TcpTransport_Transport() );
        heartbeatApp = &(CondorHeartBeat_namespace::new_CondorHeartBeat_HeartBeat(*tcp)) ;
        heartbeatApp->registerUniqueHandler(*this);
        heartbeatApp->maceInit();
    }
    virtual void stop(){
        std::cout<<"sleep finished"<<std::endl;
        heartbeatApp->maceExit();
        std::cout<<"maceExit() called"<<std::endl;
        SysUtil::sleepm(1000);
        mace::Shutdown();
        std::cout<<"scheduler halt"<<std::endl;
        delete tcp;
        delete heartbeatApp;
    }
protected:
    //MaceKey me;
    //MaceKey master;

    static HeartBeatServiceClass  *heartbeatApp;
private:
    TransportServiceClass* tcp;

};
typedef mace::map<MaceAddr, mace::set<mace::string> > ContextMapping;
HeartBeatServiceClass* ContextJobNode::heartbeatApp = NULL;
