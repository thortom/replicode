#include "unity.h"

#include <r_comp/decompiler.h>
#include <sstream>

typedef void(__stdcall * VisualizerAddNodeCallback) (uint32_t oid, const char* _group, const char* _type, const char* _name, const char* _src);
typedef void(__stdcall * VisualizerAddEdgeCallback) (const char* src, const char* target, bool is_view);



VisContext* VisContext::current = NULL;


void do_visualizer_populate(VisContext *ctx, VisualizerAddNodeCallback ancb, VisualizerAddEdgeCallback aecb) {

    if (ctx == NULL)
    {
        ::debug("do_vis_pop") << "vis ctx was null\n";
        return;
    }

    ctx->m_mem->stop();
    ::debug("do_vis_pop") << "stopped mem";

    r_comp::Image* image = ctx->m_mem->get_objects();

    // wrinkle: this seems needed to copy symbols from the original (init?) image
//    // Ensure that we get proper names
    image->object_names.symbols = ctx->m_seed_image->object_names.symbols;


    r_comp::Decompiler decompiler;
    decompiler.init(ctx->m_metadata);

    ::debug("do_vis_pop") << "m_mem:        " << (std::hex, (void*)ctx->m_mem) << (std::dec, "");
    ::debug("do_vis_pop") << "m_metadata:   " << (std::hex, (void*)ctx->m_metadata) << (std::dec, "");
    ::debug("do_vis_pop") << "m_seed_image: " << (std::hex, (void*)ctx->m_seed_image) << (std::dec, "");
    ::debug("do_vis_pop") << "image:        " << (std::hex, (void*)image) << (std::dec, "");

    uint64_t objectCount = decompiler.decompile_references(image);

    for (size_t i = 0; i < objectCount; i++) {

        std::ostringstream source;
        source.precision(2);
        decompiler.decompile_object(i, &source, 0);
        uint32_t oid = image->code_segment.objects[i]->oid;
        std::string node_name = decompiler.get_object_name(i);
        std::string type = ctx->m_metadata->classes_by_opcodes[image->code_segment.objects[i]->code[0].asOpcode()].str_opcode;
        std::string group = ""; // client can fill this?

//        ::debug("do_vis_pop") << "obj:" << oid << "," << group << "," << type << "," << node_name << ":" << source.str() << "\n";

        ancb(oid, group.c_str(), type.c_str(), node_name.c_str(), source.str().c_str());

    }

    ctx->m_mem->start(); // TODO: could restart after image gen?
    ::debug("do_vis_pop") << "restarted mem";

}

extern "C" void DLLEXPORT visualizer_populate(VisualizerAddNodeCallback ancb, VisualizerAddEdgeCallback aecb)
{
    do_visualizer_populate(VisContext::current, ancb, aecb);
}




////////////
#if false
void ReplicodeHandler::decompileImage(r_comp::Image *image)
{
    m_nodes.clear();
    m_edges.clear();

    r_comp::Decompiler decompiler;
    decompiler.init(m_metadata);

    uint64_t objectCount = decompiler.decompile_references(image);

    for (size_t i=0; i<objectCount; i++) {
        std::ostringstream source;
        source.precision(2);
        decompiler.decompile_object(i, &source, 0);
        QString nodeName = QString::fromStdString(decompiler.get_object_name(i));

        QString type = QString::fromStdString(m_metadata->classes_by_opcodes[image->code_segment.objects[i]->code[0].asOpcode()].str_opcode);

        QString group;
        if (type.startsWith("mk.")) {
            group = "passive";
        } else if (type.startsWith("ont")) {
            group = "passive";
        } else if (type.startsWith("ent")) {
            group = "passive";
        } else if (type.contains("fact")) {
            group = "passive";
        } else if (type.contains("mdl")) {
            group = "active";
        } else if (type.startsWith("cst")) {
            group = "passive";
        } else if (type.contains("pgm")) {
            group = "active";
        } else if (type.contains("grp")) {
            group = "groups";
        } else if (type.contains("perf")) {
            group = "passive";
        } else if (type.contains("cmd")) {
            group = "passive";
        } else {
            qDebug() << "Uncategorized object class" << nodeName << type;
            continue;
        }

        Node node;
        node.group = group;
        node.subgroup = type;

        node.displayName = nodeName;
        if (!nodeName.contains(type)) {
            node.displayName += " (" + type + ')';
        }

        QTextDocument *sourceDoc = new QTextDocument(QString::fromStdString(source.str()));
        new ReplicodeHighlighter(sourceDoc);
        node.sourcecode = std::shared_ptr<QTextDocument>(sourceDoc);
        m_nodes.insert(nodeName, node);

        r_code::SysObject *imageObject = image->code_segment.objects[i];
        for (size_t j=0; j<imageObject->views.size(); j++) {
            r_code::SysView *view = imageObject->views[j];
            for (size_t k=0; k<view->references.size(); k++) {
                Edge edge;
                edge.source = nodeName;
                edge.target = QString::fromStdString(decompiler.get_object_name(view->references[k]));
                edge.isView = true;
                m_edges.append(edge);
            }
        }
        for (size_t j=0; j<imageObject->references.size(); j++) {
            Edge edge;
            edge.source = nodeName;
            edge.target = QString::fromStdString(decompiler.get_object_name(imageObject->references[j]));
            m_edges.append(edge);
        }
    }
}
#endif