/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                       SSSSS  PPPP   L       AAA   Y   Y                     %
%                       SS     P   P  L      A   A   Y Y                      %
%                        SSS   PPPP   L      AAAAA    Y                       %
%                          SS  P      L      A   A    Y                       %
%                       SSSSS  P      LLLLL  A   A    Y                       %
%                                                                             %
%                                                                             %
%                 See Self-adjusting Binary Search Tree Methods               %
%                                                                             %
%                              Software Design                                %
%                                John Cristy                                  %
%                               December 2002                                 %
%                                                                             %
%                                                                             %
%  Copyright 1999-2005 ImageMagick Studio LLC, a non-profit organization      %
%  dedicated to making software imaging solutions freely available.           %
%                                                                             %
%  You may not use this file except in compliance with the License.  You may  %
%  obtain a copy of the License at                                            %
%                                                                             %
%    http://www.imagemagick.org/script/license.php                            %
%                                                                             %
%  Unless required by applicable law or agreed to in writing, software        %
%  distributed under the License is distributed on an "AS IS" BASIS,          %
%  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   %
%  See the License for the specific language governing permissions and        %
%  limitations under the License.                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  This module implements the standard handy splay-tree methods for storing and
%  retrieving large numbers of data elements.  It is loosely based on the Java
%  implementation of these algorithms.
%
*/

/*
  Include declarations.
*/
#include "magick/studio.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/log.h"
#include "magick/memory_.h"
#include "magick/splay-tree.h"
#include "magick/semaphore.h"
#include "magick/string_.h"

/*
  Define declarations.
*/
#define MaxSplayTreeDepth  2048

/*
  Typedef declarations.
*/
typedef struct _NodeInfo
{
  void
    *key;

  void
    *value;

  struct _NodeInfo
    *left,
    *right;
} NodeInfo;

struct _SplayTreeInfo
{
  NodeInfo
    *root;

  int
    (*compare)(const void *,const void *);

  void
    *(*relinquish_key)(void *),
    *(*relinquish_value)(void *);

  unsigned long
    depth;

  MagickBooleanType
    balance;

  void
    *key,
    *next;

  unsigned long
    nodes;

  MagickBooleanType
    debug;

  SemaphoreInfo
    *semaphore;

  unsigned long
    signature;
};

/*
  Forward declarations.
*/
static int
  IterateOverSplayTree(SplayTreeInfo *,int (*)(NodeInfo *,const void *),
    const void *);

static void
  SplaySplayTree(SplayTreeInfo *,const void *);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   A d d V a l u e T o S p l a y T r e e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AddValueToSplayTree() adds a value to the splay-tree.
%
%  The format of the AddValueToSplayTree method is:
%
%      MagickBooleanType AddValueToSplayTree(SplayTreeInfo *splay_info,
%        const void *key,const void *value)
%
%  A description of each parameter follows:
%
%    o splay_info: The splay-tree info.
%
%    o key: The key.
%
%    o value: The value.
%
*/
MagickExport MagickBooleanType AddValueToSplayTree(SplayTreeInfo *splay_info,
  const void *key,const void *value)
{
  int
    compare;

  register NodeInfo
    *node;

  AcquireSemaphoreInfo(&splay_info->semaphore);
  SplaySplayTree(splay_info,key);
  compare=0;
  if (splay_info->root != (NodeInfo *) NULL)
    {
      if (splay_info->compare != (int (*)(const void *,const void *)) NULL)
        compare=splay_info->compare(splay_info->root->key,key);
      else
        compare=(splay_info->root->key > key) ? 1 :
          ((splay_info->root->key < key) ? -1 : 0);
      if (compare == 0)
        {
          if ((splay_info->relinquish_value != (void *(*)(void *)) NULL) &&
              (splay_info->root->value != (void *) NULL))
            splay_info->root->value=splay_info->relinquish_value(
              splay_info->root->value);
          if ((splay_info->relinquish_key != (void *(*)(void *)) NULL) &&
              (splay_info->root->key != (void *) NULL))
            splay_info->root->key=splay_info->relinquish_key(
              splay_info->root->key);
          splay_info->root->key=(void *) key;
          splay_info->root->value=(void *) value;
          RelinquishSemaphoreInfo(splay_info->semaphore);
          return(MagickTrue);
        }
    }
  node=(NodeInfo *) AcquireMagickMemory(sizeof(*node));
  if (node == (NodeInfo *) NULL)
    ThrowMagickFatalException(ResourceLimitFatalError,"MemoryAllocationFailed",
      strerror(errno));
  node->key=(void *) key;
  node->value=(void *) value;
  if (splay_info->root == (NodeInfo *) NULL)
    {
      node->left=(NodeInfo *) NULL;
      node->right=(NodeInfo *) NULL;
    }
  else
    if (compare < 0)
      {
        node->left=splay_info->root;
        node->right=node->left->right;
        node->left->right=(NodeInfo *) NULL;
      }
    else
      {
        node->right=splay_info->root;
        node->left=node->right->left;
        node->right->left=(NodeInfo *) NULL;
      }
  splay_info->root=node;
  splay_info->key=(void *) NULL;
  splay_info->nodes++;
  RelinquishSemaphoreInfo(splay_info->semaphore);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   B a l a n c e S p l a y T r e e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  BalanceSplayTree() balances the splay-tree.
%
%  The format of the BalanceSplayTree method is:
%
%      void *BalanceSplayTree(SplayTreeInfo *splay_info,const void *key)
%
%  A description of each parameter follows:
%
%    o splay_info: The splay-tree info.
%
%    o key: The key.
%
*/

static NodeInfo *LinkSplayTreeNodes(NodeInfo **nodes,unsigned long low,
  unsigned long high)
{
  register NodeInfo
    *node;

  unsigned long
    bisect;

  bisect=low+(high-low)/2;
  node=nodes[bisect];
  if ((low+1) > bisect)
    node->left=(NodeInfo *) NULL;
  else
    node->left=LinkSplayTreeNodes(nodes,low,bisect-1);
  if ((bisect+1) > high)
    node->right=(NodeInfo *) NULL;
  else
    node->right=LinkSplayTreeNodes(nodes,bisect+1,high);
  return(node);
}

static int SplayTreeToNodeArray(NodeInfo *node,const void *nodes)
{
  register const NodeInfo
    ***p;

  p=(const NodeInfo ***) nodes;
  *(*p)=node;
  (*p)++;
  return(0);
}

static void BalanceSplayTree(SplayTreeInfo *splay_info)
{
  NodeInfo
    **node,
    **nodes;

  if (splay_info->nodes <= 2)
    {
      splay_info->balance=MagickFalse;
      return;
    }
  nodes=(NodeInfo **) AcquireMagickMemory(splay_info->nodes*sizeof(*nodes));
  if (nodes == (NodeInfo **) NULL)
    ThrowMagickFatalException(ResourceLimitFatalError,"MemoryAllocationFailed",
      strerror(errno));
  node=nodes;
  (void) IterateOverSplayTree(splay_info,SplayTreeToNodeArray,
    (const void *) &node);
  splay_info->root=LinkSplayTreeNodes(nodes,0,splay_info->nodes-1);
  splay_info->balance=MagickFalse;
  nodes=(NodeInfo **) RelinquishMagickMemory(nodes);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C o m p a r e S p l a y T r e e S t r i n g                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Specify the CompareSplayTreeString() method in NewSplayTree() to find a node
%  in a splay-tree based on the contents of a string.
%
%  The format of the CompareSplayTreeString method is:
%
%      int CompareSplayTreeString(const void *target,const void *source)
%
%  A description of each parameter follows:
%
%    o target: The target string.
%
%    o source: The source string.
%
*/
MagickExport int CompareSplayTreeString(const void *target,const void *source)
{
  const char
    *p,
    *q;

  p=(const char *) target;
  q=(const char *) source;
  return(LocaleCompare(p,q));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C o m p a r e S p l a y T r e e S t r i n g I n f o                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Specify the CompareSplayTreeStringInfo() method in NewSplayTree() to find a
%  node in a splay-tree based on the contents of a string.
%
%  The format of the CompareSplayTreeStringInfo method is:
%
%      int CompareSplayTreeStringInfo(const void *target,const void *source)
%
%  A description of each parameter follows:
%
%    o target: The target string.
%
%    o source: The source string.
%
*/
MagickExport int CompareSplayTreeStringInfo(const void *target,
  const void *source)
{
  const StringInfo
    *p,
    *q;

  p=(const StringInfo *) target;
  q=(const StringInfo *) source;
  return(CompareStringInfo(p,q));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D e s t r o y S p l a y T r e e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroySplayTree() destroys the splay-tree.
%
%  The format of the DestroySplayTree method is:
%
%      SplayTreeInfo *DestroySplayTree(SplayTreeInfo *splay_info)
%
%  A description of each parameter follows:
%
%    o splay_info: The splay info.
%
*/
MagickExport SplayTreeInfo *DestroySplayTree(SplayTreeInfo *splay_info)
{
  NodeInfo
    *node;

  register NodeInfo
    *active,
    *pend;

  AcquireSemaphoreInfo(&splay_info->semaphore);
  if (splay_info->root != (NodeInfo *) NULL)
    {
      if ((splay_info->relinquish_value != (void *(*)(void *)) NULL) &&
          (splay_info->root->value != (void *) NULL))
        splay_info->root->value=splay_info->relinquish_value(
          splay_info->root->value);
      if ((splay_info->relinquish_key != (void *(*)(void *)) NULL) &&
          (splay_info->root->key != (void *) NULL))
        splay_info->root->key=splay_info->relinquish_key(splay_info->root->key);
      splay_info->root->key=(void *) NULL;
      for (pend=splay_info->root; pend != (NodeInfo *) NULL; )
      {
        active=pend;
        for (pend=(NodeInfo *) NULL; active != (NodeInfo *) NULL; )
        {
          if (active->left != (NodeInfo *) NULL)
            {
              if ((splay_info->relinquish_value != (void *(*)(void *)) NULL) &&
                  (active->left->value != (void *) NULL))
                active->left->value=splay_info->relinquish_value(
                  active->left->value);
              if ((splay_info->relinquish_key != (void *(*)(void *)) NULL) &&
                  (active->left->key != (void *) NULL))
                active->left->key=splay_info->relinquish_key(active->left->key);
              active->left->key=(void *) pend;
              pend=active->left;
            }
          if (active->right != (NodeInfo *) NULL)
            {
              if ((splay_info->relinquish_value != (void *(*)(void *)) NULL) &&
                  (active->right->value != (void *) NULL))
                active->right->value=splay_info->relinquish_value(
                  active->right->value);
              if ((splay_info->relinquish_key != (void *(*)(void *)) NULL) &&
                  (active->right->key != (void *) NULL))
                active->right->key=splay_info->relinquish_key(
                  active->right->key);
              active->right->key=(void *) pend;
              pend=active->right;
            }
          node=active;
          active=(NodeInfo *) node->key;
          node=(NodeInfo *) RelinquishMagickMemory(node);
        }
      }
    }
  splay_info->signature=(~MagickSignature);
  RelinquishSemaphoreInfo(splay_info->semaphore);
  splay_info->semaphore=DestroySemaphoreInfo(splay_info->semaphore);
  splay_info=(SplayTreeInfo *) RelinquishMagickMemory(splay_info);
  return(splay_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t N e x t K e y I n S p l a y T r e e                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetNextKeyInSplayTree() gets the next key in the splay-tree.
%
%  The format of the GetNextKeyInSplayTree method is:
%
%      void *GetNextKeyInSplayTree(SplayTreeInfo *splay_info)
%
%  A description of each parameter follows:
%
%    o splay_info: The splay info.
%
%    o key: The key.
%
*/
MagickExport void *GetNextKeyInSplayTree(SplayTreeInfo *splay_info)
{
  register NodeInfo
    *node;

  assert(splay_info != (SplayTreeInfo *) NULL);
  assert(splay_info->signature == MagickSignature);
  if (splay_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  if ((splay_info->root == (NodeInfo *) NULL) ||
      (splay_info->next == (void *) NULL))
    return((void *) NULL);
  AcquireSemaphoreInfo(&splay_info->semaphore);
  SplaySplayTree(splay_info,splay_info->next);
  splay_info->next=(void *) NULL;
  node=splay_info->root->right;
  if (node != (NodeInfo *) NULL)
    {
      while (node->left != (NodeInfo *) NULL)
        node=node->left;
      splay_info->next=node->key;
    }
  RelinquishSemaphoreInfo(splay_info->semaphore);
  return(splay_info->root->key);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t N e x t V a l u e I n S p l a y T r e e                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetNextValueInSplayTree() gets the next value in the splay-tree.
%
%  The format of the GetNextValueInSplayTree method is:
%
%      void *GetNextValueInSplayTree(SplayTreeInfo *splay_info)
%
%  A description of each parameter follows:
%
%    o splay_info: The splay info.
%
%    o key: The key.
%
*/
MagickExport void *GetNextValueInSplayTree(SplayTreeInfo *splay_info)
{
  register NodeInfo
    *node;

  assert(splay_info != (SplayTreeInfo *) NULL);
  assert(splay_info->signature == MagickSignature);
  if (splay_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  if ((splay_info->root == (NodeInfo *) NULL) ||
      (splay_info->next == (void *) NULL))
    return((void *) NULL);
  AcquireSemaphoreInfo(&splay_info->semaphore);
  SplaySplayTree(splay_info,splay_info->next);
  splay_info->next=(void *) NULL;
  node=splay_info->root->right;
  if (node != (NodeInfo *) NULL)
    {
      while (node->left != (NodeInfo *) NULL)
        node=node->left;
      splay_info->next=node->key;
    }
  RelinquishSemaphoreInfo(splay_info->semaphore);
  return(splay_info->root->value);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t V a l u e F r o m S p l a y T r e e                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetValueFromSplayTree() gets a value from the splay-tree by its key.
%
%  The format of the GetValueFromSplayTree method is:
%
%      void *GetValueFromSplayTree(SplayTreeInfo *splay_info,const void *key)
%
%  A description of each parameter follows:
%
%    o splay_info: The splay info.
%
%    o key: The key.
%
*/
MagickExport void *GetValueFromSplayTree(SplayTreeInfo *splay_info,
  const void *key)
{
  int
    compare;

  assert(splay_info != (SplayTreeInfo *) NULL);
  assert(splay_info->signature == MagickSignature);
  if (splay_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  if (splay_info->root == (NodeInfo *) NULL)
    return((void *) NULL);
  AcquireSemaphoreInfo(&splay_info->semaphore);
  SplaySplayTree(splay_info,key);
  if (splay_info->compare != (int (*)(const void *,const void *)) NULL)
    compare=splay_info->compare(splay_info->root->key,key);
  else
    compare=(splay_info->root->key > key) ? 1 :
      ((splay_info->root->key < key) ? -1 : 0);
  if (compare != 0)
    {
      RelinquishSemaphoreInfo(splay_info->semaphore);
      return((void *) NULL);
    }
  RelinquishSemaphoreInfo(splay_info->semaphore);
  return(splay_info->root->value);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t N u m b e r O f N o d e s I n S p l a y T r e e                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetNumberOfNodesInSplayTree() returns the number of nodes in the splay-tree.
%
%  The format of the GetNumberOfNodesInSplayTree method is:
%
%      unsigned long GetNumberOfNodesInSplayTree(
%        const SplayTreeInfo *splay_tree)
%
%  A description of each parameter follows:
%
%    o splay_info: The splay info.
%
*/
MagickExport unsigned long GetNumberOfNodesInSplayTree(
  const SplayTreeInfo *splay_info)
{
  assert(splay_info != (SplayTreeInfo *) NULL);
  assert(splay_info->signature == MagickSignature);
  if (splay_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  return(splay_info->nodes);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I t e r a t e O v e r S p l a y T r e e                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IterateOverSplayTree() iterates over the splay-tree.
%
%  The format of the IterateOverSplayTree method is:
%
%      int IterateOverSplayTree(SplayTreeInfo *splay_info,
%        int (*method)(NodeInfo *,void *),const void *value)
%
%  A description of each parameter follows:
%
%    o splay_info: The splay-tree info.
%
%    o method: The method.
%
%    o value: The value.
%
*/
static int IterateOverSplayTree(SplayTreeInfo *splay_info,
  int (*method)(NodeInfo *,const void *),const void *value)
{
  typedef enum
  {
    LeftTransition,
    RightTransition,
    DownTransition,
    UpTransition
  } TransitionType;

  int
    status;

  MagickBooleanType
    final_transition;

  NodeInfo
    **nodes;

  register long
    i;

  register NodeInfo
    *node;

  TransitionType
    transition;

  unsigned char
    *transitions;

  if (splay_info->root == (NodeInfo *) NULL)
    return(0);
  nodes=(NodeInfo **) AcquireMagickMemory(splay_info->nodes*sizeof(*nodes));
  transitions=(unsigned char *)
    AcquireMagickMemory(splay_info->nodes*sizeof(*transitions));
  if ((nodes == (NodeInfo **) NULL) || (transitions == (unsigned char *) NULL))
    ThrowMagickFatalException(ResourceLimitFatalError,"MemoryAllocationFailed",
      strerror(errno));
  status=0;
  final_transition=MagickFalse;
  nodes[0]=splay_info->root;
  transitions[0]=(unsigned char) LeftTransition;
  for (i=0; final_transition == MagickFalse; )
  {
    node=nodes[i];
    transition=(TransitionType) transitions[i];
    switch (transition)
    {
      case LeftTransition:
      {
        transitions[i]=(unsigned char) DownTransition;
        if (node->left == (NodeInfo *) NULL)
          break;
        i++;
        nodes[i]=node->left;
        transitions[i]=(unsigned char) LeftTransition;
        break;
      }
      case RightTransition:
      {
        transitions[i]=(unsigned char) UpTransition;
        if (node->right == (NodeInfo *) NULL)
          break;
        i++;
        nodes[i]=node->right;
        transitions[i]=(unsigned char) LeftTransition;
        break;
      }
      case DownTransition:
      default:
      {
        transitions[i]=(unsigned char) RightTransition;
        status=(*method)(node,value);
        if (status != 0)
          final_transition=MagickTrue;
        break;
      }
      case UpTransition:
      {
        if (i == 0)
          {
            final_transition=MagickTrue;
            break;
          }
        i--;
        break;
      }
    }
  }
  nodes=(NodeInfo **) RelinquishMagickMemory(nodes);
  transitions=(unsigned char *) RelinquishMagickMemory(transitions);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   N e w S p l a y T r e e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  NewSplayTree() returns a pointer to a SplayTreeInfo structure initialized
%  to default values.
%
%  The format of the NewSplayTree method is:
%
%      SplayTreeInfo *NewSplayTree(int (*compare)(const void *,const void *),
%        void *(*relinquish_key)(void *),void *(*relinquish_value)(void *))
%
%  A description of each parameter follows:
%
%    o compare: The compare method.
%
%    o relinquish_key: The key deallocation method, typically
%      RelinquishMagickMemory(), called whenever a key is removed from the
%      splay-tree.
%
%    o relinquish_value: The value deallocation method;  typically
%      RelinquishMagickMemory(), called whenever a value object is removed from
%      the splay-tree.
%
*/
MagickExport SplayTreeInfo *NewSplayTree(
  int (*compare)(const void *,const void *),void *(*relinquish_key)(void *),
  void *(*relinquish_value)(void *))
{
  SplayTreeInfo
    *splay_info;

  splay_info=(SplayTreeInfo *) AcquireMagickMemory(sizeof(*splay_info));
  if (splay_info == (SplayTreeInfo *) NULL)
    ThrowMagickFatalException(ResourceLimitFatalError,"MemoryAllocationFailed",
      strerror(errno));
  (void) ResetMagickMemory(splay_info,0,sizeof(*splay_info));
  splay_info->root=(NodeInfo *) NULL;
  splay_info->compare=compare;
  splay_info->relinquish_key=relinquish_key;
  splay_info->relinquish_value=relinquish_value;
  splay_info->depth=0;
  splay_info->balance=MagickFalse;
  splay_info->key=(void *) NULL;
  splay_info->next=(void *) NULL;
  splay_info->nodes=0;
  splay_info->debug=IsEventLogging();
  splay_info->semaphore=(SemaphoreInfo *) NULL;
  splay_info->signature=MagickSignature;
  return(splay_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e m o v e N o d e B y V a l u e F r o m S p l a y T r e e               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RemoveNodeByValueFromSplayTree() removes a node by value from the splay-tree.
%
%  The format of the RemoveNodeByValueFromSplayTree method is:
%
%      MagickBooleanType RemoveNodeByValueFromSplayTree(
%        SplayTreeInfo *splay_info,const void *value)
%
%  A description of each parameter follows:
%
%    o splay_info: The splay-tree info.
%
%    o value: The value.
%
*/

static void *GetFirstSplayTreeNode(SplayTreeInfo *splay_info)
{
  register NodeInfo
    *node;

  node=splay_info->root;
  if (splay_info->root == (NodeInfo *) NULL)
    return((NodeInfo *) NULL);
  while (node->left != (NodeInfo *) NULL)
    node=node->left;
  return(node->key);
}

MagickExport MagickBooleanType RemoveNodeByValueFromSplayTree(
  SplayTreeInfo *splay_info,const void *value)
{
  register NodeInfo
    *next,
    *node;

  assert(splay_info != (SplayTreeInfo *) NULL);
  assert(splay_info->signature == MagickSignature);
  if (splay_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  if (splay_info->root == (NodeInfo *) NULL)
    return(MagickFalse);
  AcquireSemaphoreInfo(&splay_info->semaphore);
  next=(NodeInfo *) GetFirstSplayTreeNode(splay_info);
  while (next != (NodeInfo *) NULL)
  {
    SplaySplayTree(splay_info,next);
    next=(NodeInfo *) NULL;
    node=splay_info->root->right;
    if (node != (NodeInfo *) NULL)
      {
        while (node->left != (NodeInfo *) NULL)
          node=node->left;
        next=(NodeInfo *) node->key;
      }
    if (splay_info->root->value == value)
      {
        RelinquishSemaphoreInfo(splay_info->semaphore);
        return(RemoveNodeFromSplayTree(splay_info,splay_info->root->key));
      }
  }
  RelinquishSemaphoreInfo(splay_info->semaphore);
  return(MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e m o v e N o d e F r o m S p l a y T r e e                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RemoveNodeFromSplayTree() removes a node from the splay-tree.
%
%  The format of the RemoveNodeFromSplayTree method is:
%
%      MagickBooleanType RemoveNodeFromSplayTree(SplayTreeInfo *splay_info,
%        const void *key)
%
%  A description of each parameter follows:
%
%    o splay_info: The splay-tree info.
%
%    o key: The key.
%
*/
MagickExport MagickBooleanType RemoveNodeFromSplayTree(
  SplayTreeInfo *splay_info,const void *key)
{
  int
    compare;

  register NodeInfo
    *left,
    *right;

  assert(splay_info != (SplayTreeInfo *) NULL);
  assert(splay_info->signature == MagickSignature);
  if (splay_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  if (splay_info->root == (NodeInfo *) NULL)
    return(MagickFalse);
  AcquireSemaphoreInfo(&splay_info->semaphore);
  SplaySplayTree(splay_info,key);
  splay_info->key=(void *) NULL;
  if (splay_info->compare != (int (*)(const void *,const void *)) NULL)
    compare=splay_info->compare(splay_info->root->key,key);
  else
    compare=(splay_info->root->key > key) ? 1 :
      ((splay_info->root->key < key) ? -1 : 0);
  if (compare != 0)
    {
      RelinquishSemaphoreInfo(splay_info->semaphore);
      return(MagickFalse);
    }
  left=splay_info->root->left;
  right=splay_info->root->right;
  if ((splay_info->relinquish_value != (void *(*)(void *)) NULL) &&
      (splay_info->root->value != (void *) NULL))
    splay_info->root->value=splay_info->relinquish_value(
      splay_info->root->value);
  if ((splay_info->relinquish_key != (void *(*)(void *)) NULL) &&
      (splay_info->root->key != (void *) NULL))
    splay_info->root->key=splay_info->relinquish_key(splay_info->root->key);
  splay_info->root=(NodeInfo *) RelinquishMagickMemory(splay_info->root);
  splay_info->nodes--;
  if (left == (NodeInfo *) NULL)
    {
      splay_info->root=right;
      RelinquishSemaphoreInfo(splay_info->semaphore);
      return(MagickTrue);
    }
  splay_info->root=left;
  if (right != (NodeInfo *) NULL)
    {
      while (left->right != (NodeInfo *) NULL)
        left=left->right;
      left->right=right;
    }
  RelinquishSemaphoreInfo(splay_info->semaphore);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e s e t S p l a y T r e e I t e r a t o r                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ResetSplayTreeIterator() resets the splay-tree iterator.  Use it in
%  conjunction with GetNextValueInSplayTree() to iterate over all the nodes in
%  the splay-tree.
%
%  The format of the ResetSplayTreeIterator method is:
%
%      ResetSplayTreeIterator(SplayTreeInfo *splay_info)
%
%  A description of each parameter follows:
%
%    o splay_info: The splay info.
%
*/
MagickExport void ResetSplayTreeIterator(SplayTreeInfo *splay_info)
{
  assert(splay_info != (SplayTreeInfo *) NULL);
  assert(splay_info->signature == MagickSignature);
  if (splay_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  splay_info->next=GetFirstSplayTreeNode(splay_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S p l a y S p l a y T r e e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SplaySplayTree() splays the splay-tree.
%
%  The format of the SplaySplayTree method is:
%
%      void SplaySplayTree(SplayTreeInfo *splay_info,const void *key,
%        NodeInfo **node,NodeInfo **parent,NodeInfo **grandparent)
%
%  A description of each parameter follows:
%
%    o splay_info: The splay-tree info.
%
%    o key: The key.
%
%    o node: The node.
%
%    o parent: The parent node.
%
%    o grandparent: The grandparent node.
%
*/

static NodeInfo *Splay(SplayTreeInfo *splay_info,const void *key,
  NodeInfo **node,NodeInfo **parent,NodeInfo **grandparent)
{
  int
    compare;

  NodeInfo
    **next;

  register NodeInfo
    *n,
    *p;

  n=(*node);
  if (n == (NodeInfo *) NULL)
    return(*parent);
  if (splay_info->compare != (int (*)(const void *,const void *)) NULL)
    compare=splay_info->compare(n->key,key);
  else
    compare=(n->key > key) ? 1 : ((n->key < key) ? -1 : 0);
  next=(NodeInfo **) NULL;
  if (compare > 0)
    next=(&n->left);
  else
    if (compare < 0)
      next=(&n->right);
  if (next != (NodeInfo **) NULL)
    {
      if (splay_info->depth >= MaxSplayTreeDepth)
        {
          splay_info->balance=MagickTrue;
          return(n);
        }
      splay_info->depth++;
      n=Splay(splay_info,key,next,node,parent);
      splay_info->depth--;
      if ((*node != n) || (splay_info->balance != MagickFalse))
        return(n);
    }
  if (parent == (NodeInfo **) NULL)
    return(n);
  if (grandparent == (NodeInfo **) NULL)
    {
      if (n == (*parent)->left)
        {
          *node=n->right;
          n->right=(*parent);
        }
      else
        {
          *node=n->left;
          n->left=(*parent);
        }
      *parent=n;
      return(n);
    }
  if ((n == (*parent)->left) && (*parent == (*grandparent)->left))
    {
      p=(*parent);
      (*grandparent)->left=p->right;
      p->right=(*grandparent);
      p->left=n->right;
      n->right=p;
      *grandparent=n;
      return(n);
    }
  if ((n == (*parent)->right) && (*parent == (*grandparent)->right))
    {
      p=(*parent);
      (*grandparent)->right=p->left;
      p->left=(*grandparent);
      p->right=n->left;
      n->left=p;
      *grandparent=n;
      return(n);
    }
  if (n == (*parent)->left)
    {
      (*parent)->left=n->right;
      n->right=(*parent);
      (*grandparent)->right=n->left;
      n->left=(*grandparent);
      *grandparent=n;
      return(n);
    }
  (*parent)->right=n->left;
  n->left=(*parent);
  (*grandparent)->left=n->right;
  n->right=(*grandparent);
  *grandparent=n;
  return(n);
}

static void SplaySplayTree(SplayTreeInfo *splay_info,const void *key)
{
  if (splay_info->root == (NodeInfo *) NULL)
    return;
  if (splay_info->key != (void *) NULL)
    {
      int
        compare;

      if (splay_info->compare != (int (*)(const void *,const void *)) NULL)
        compare=splay_info->compare(splay_info->root->key,key);
      else
        compare=(splay_info->key > key) ? 1 :
          ((splay_info->key < key) ? -1 : 0);
      if (compare == 0)
        return;
    }
  splay_info->depth=0;
  (void) Splay(splay_info,key,&splay_info->root,(NodeInfo **) NULL,
    (NodeInfo **) NULL);
  if (splay_info->balance != MagickFalse)
    {
      BalanceSplayTree(splay_info);
      splay_info->depth=0;
      (void) Splay(splay_info,key,&splay_info->root,(NodeInfo **) NULL,
        (NodeInfo **) NULL);
      if (splay_info->balance != MagickFalse)
        ThrowMagickFatalException(ResourceLimitFatalError,
          "MemoryAllocationFailed",strerror(errno));
    }
  splay_info->key=(void *) key;
}
