/* -*- Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2011 University of California, Los Angeles
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Alexander Afanasyev <alexander.afanasyev@ucla.edu>
 */

#include "ccnb-parser-no-argu-depth-first-visitor.h"

#include "ns3/ccnb-parser-blob.h"
#include "ns3/ccnb-parser-udata.h"
#include "ns3/ccnb-parser-tag.h"
#include "ns3/ccnb-parser-dtag.h"
#include "ns3/ccnb-parser-attr.h"
#include "ns3/ccnb-parser-dattr.h"
#include "ns3/ccnb-parser-ext.h"

#include <boost/foreach.hpp>

namespace ns3 {
namespace CcnbParser {

boost::any
NoArguDepthFirstVisitor::visit (Blob &n)
{
  // Buffer n.m_blob;
  return n.m_blob;
}
 
boost::any
NoArguDepthFirstVisitor::visit (Udata &n)
{
  // std::string n.m_udata;
  return n.m_udata;
}
 
boost::any
NoArguDepthFirstVisitor::visit (Tag &n)
{
  // uint32_t n.m_dtag;
  // std::list<Ptr<Block> > n.m_attrs;
  // std::list<Ptr<Block> > n.m_nestedBlocks;
  BOOST_FOREACH (Ptr<Block> block, n.m_attrs)
    {
      block->accept (*this);
    }
  BOOST_FOREACH (Ptr<Block> block, n.m_nestedBlocks)
    {
      block->accept (*this);
    }
  return boost::any ();
}
 
boost::any
NoArguDepthFirstVisitor::visit (Dtag &n)
{
  // uint32_t n.m_dtag;
  // std::list<Ptr<Block> > n.m_attrs;
  // std::list<Ptr<Block> > n.m_nestedBlocks;
  BOOST_FOREACH (Ptr<Block> block, n.m_attrs)
    {
      block->accept (*this);
    }
  BOOST_FOREACH (Ptr<Block> block, n.m_nestedBlocks)
    {
      block->accept (*this);
    }
  return boost::any();
}
 
boost::any
NoArguDepthFirstVisitor::visit (Attr &n)
{
  // std::string n.m_attr;
  // Ptr<Udata> n.m_value;
  return boost::any ();
}
 
boost::any
NoArguDepthFirstVisitor::visit (Dattr &n)
{
  // uint32_t n.m_dattr;
  // Ptr<Udata> n.m_value;
  return boost::any ();
}
 
boost::any
NoArguDepthFirstVisitor::visit (Ext &n)
{
  // uint64_t n.m_extSubtype;
  return n.m_extSubtype;
}

}
}
