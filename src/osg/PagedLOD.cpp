#include <osg/PagedLOD>

using namespace osg;

PagedLOD::PagedLOD(const PagedLOD& plod,const CopyOp& copyop):
    LOD(plod,copyop)
{
}

void PagedLOD::traverse(NodeVisitor& nv)
{

    double timeStamp = nv.getFrameStamp()?nv.getFrameStamp()->getReferenceTime():0.0;
    bool updateTimeStamp = nv.getVisitorType()==osg::NodeVisitor::CULL_VISITOR;

    switch(nv.getTraversalMode())
    {
        case(NodeVisitor::TRAVERSE_ALL_CHILDREN):
            std::for_each(_children.begin(),_children.end(),NodeAcceptOp(nv));
            break;
        case(NodeVisitor::TRAVERSE_ACTIVE_CHILDREN):
        {
            float distance = nv.getDistanceToEyePoint(getCenter(),true);
            
            // first check to see if any children can be directly traversed.
            unsigned int numChildren = osg::minimum(_children.size(),numChildren=_rangeList.size());
            bool childTraversed = false;
            for(unsigned int i=0;i<numChildren;++i)
            {    
                if (_rangeList[i].first<=distance && distance<_rangeList[i].second)
                {
                    if (updateTimeStamp) _timeStampList[i]=timeStamp;

                    //std::cout<<"PagedLOD::traverse() - Selecting child "<<i<<std::endl;
                    _children[i]->accept(nv);
                    childTraversed = true;
                }
            }
            
            if (!childTraversed)
            {
                //std::cout<<"PagedLOD::traverse() - falling back "<<std::endl;
                // select the last valid child.
                if (numChildren>0)
                {
                    //std::cout<<"    to child "<<numChildren-1<<std::endl;
                    if (updateTimeStamp) _timeStampList[numChildren-1]=timeStamp;
                    _children[numChildren-1]->accept(nv);
                }
                
                // now request the loading of the next unload child.
                if (nv.getDatabaseRequestHandler() && numChildren<_fileNameList.size())
                {
                    //std::cout<<"    requesting child "<<_fileNameList[numChildren]<<std::endl;
                    nv.getDatabaseRequestHandler()->requestNodeFile(_fileNameList[numChildren],this);
                }
                
                
            }
            
            
           break;
        }
        default:
            break;
    }
}

bool PagedLOD::addChild( Node *child )
{
    if (LOD::addChild(child))
    {
        if (_children.size()>_fileNameList.size()) _fileNameList.resize(_children.size());
        if (_children.size()>_timeStampList.size()) _timeStampList.resize(_children.size(),0);
        return true;
    }
    return false;
}

bool PagedLOD::addChild(Node *child, float min, float max)
{
    if (LOD::addChild(child,min,max))
    {
        if (_children.size()>_fileNameList.size()) _fileNameList.resize(_children.size());
        if (_children.size()>_timeStampList.size()) _timeStampList.resize(_children.size(),0.0);

        return true;
    }
    return false;
}


bool PagedLOD::addChild(Node *child, float min, float max,const std::string& filename)
{
    if (LOD::addChild(child,min,max))
    {
        if (_children.size()>_fileNameList.size()) _fileNameList.resize(_children.size());
        if (_children.size()>_timeStampList.size()) _timeStampList.resize(_children.size(),0.0);

        _fileNameList[_children.size()-1] = filename;

        return true;
    }
    return false;
}

bool PagedLOD::removeChild( Node *child )
{
    // find the child's position.
    unsigned int pos=getChildIndex(child);
    if (pos==_children.size()) return false;
    
    _rangeList.erase(_rangeList.begin()+pos);
    _fileNameList.erase(_fileNameList.begin()+pos);
    _timeStampList.erase(_timeStampList.begin()+pos);
    
    return Group::removeChild(child);    
}

void PagedLOD::setFileName(unsigned int childNo, const std::string& filename)
{
    if (childNo>=_fileNameList.size()) _fileNameList.resize(childNo+1);
    _fileNameList[childNo] = filename;
}

void PagedLOD::setTimeStamp(unsigned int childNo, double timeStamp)
{
    if (childNo>=_timeStampList.size()) _timeStampList.resize(childNo+1,0.0);
    _timeStampList[childNo] = timeStamp;
}

void PagedLOD::removeExpiredChildren(double expiryTime)
{
    for(unsigned int i=_children.size();i>0;)
    {
        --i;
        if (!_fileNameList[i].empty() && _timeStampList[i]<expiryTime)
        {
            //std::cout<<"Removing child "<<_children[i].get()<<std::endl;
            Group::removeChild(_children[i].get());
        }
    }
}
