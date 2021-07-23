# GRUPO 54
# Sara Queimado 52806
# Diogo Fernandes 52802
# Eduardo Salvadinha 52768 

SRCDIR=source
INCDIR=include
BINDIR=binary
OBJDIR=object
LIBDIR=lib
INCFLAGS=-I$(INCDIR)
LD = ld -r

LIBS= -lrt -lpthread -pthread

data.o = data.h
entry.o = entry.h
tree.o = tree.h tree-private.h
serialization.o = serialization.h
tree_skel.o = tree_skel.h
network_server.o = network_server.h
network_client.o = network_client.h
client_stub.o = client_stub.h
tree_client.o = tree_client.h
tree_server.o = tree_server.h

all:
	make client-lib.o
	make tree-client
	make tree-server

#compile proto and move files to the right folders
proto:
	protoc --c_out=. sdmessage.proto 
	mv sdmessage.pb-c.h $(INCDIR)
	mv sdmessage.pb-c.c $(SRCDIR)

%.o: $(SRCDIR)/%.c $($@)
	gcc -g -Wall -iquote $(INCDIR) -o $(OBJDIR)/$@ -c $< -lzookeeper_mt

#ld -r object/data.o object/entry.o object/client_stub.o object/network_client.o /usr/local/lib/libprotobuf-c.a -o object/client-lib.o
client-lib.o: proto data.o entry.o message.o network_client.o client_stub.o sdmessage.pb-c.o
	$(LD) $(OBJDIR)/data.o $(OBJDIR)/entry.o $(OBJDIR)/message.o $(OBJDIR)/network_client.o $(OBJDIR)/client_stub.o $(OBJDIR)/sdmessage.pb-c.o /usr/local/lib/libprotobuf-c.a -o $(LIBDIR)/client-lib.o

sdmessage.pb-c.o:
	gcc -g -Wall $(LIBS) -iquote $(INCDIR) -o $(OBJDIR)/sdmessage.pb-c.o -c $(SRCDIR)/sdmessage.pb-c.c

tree-client: client-lib.o tree_client.o
	gcc -g -Wall $(LIBS) -iquote $(INCDIR) $(LIBDIR)/client-lib.o $(OBJDIR)/tree_client.o -o $(BINDIR)/tree-client -lzookeeper_mt

tree-server: proto data.o entry.o tree.o message.o client_stub.o network_client.o network_server.o tree_skel.o sdmessage.pb-c.o tree_server.o
	gcc -g -Wall $(LIBS) -pthread -iquote $(INCDIR) $(OBJDIR)/data.o $(OBJDIR)/entry.o $(OBJDIR)/message.o $(OBJDIR)/client_stub.o $(OBJDIR)/network_client.o $(OBJDIR)/tree.o $(OBJDIR)/network_server.o $(OBJDIR)/tree_skel.o $(OBJDIR)/sdmessage.pb-c.o /usr/local/lib/libprotobuf-c.a $(OBJDIR)/tree_server.o -o $(BINDIR)/tree-server -lzookeeper_mt

clean:
	#remove all objects
	rm -f $(OBJDIR)/*.o
	#remove all executables
	rm -f $(BINDIR)/t*
	#remove lib
	rm -f $(LIBDIR)/*.o
	#remove proto files
	rm -f $(SRCDIR)/*pb-c.* $(INCDIR)/*pb-c.* 
	#remove valgrind logs
	rm -f valgrind-client.txt
	rm -f valgrind-server1.txt
	rm -f valgrind-server2.txt
	rm -f valgrind-server3.txt
	
server1:
	./$(BINDIR)/tree-server 2000 127.0.0.1:2181

server2:
	./$(BINDIR)/tree-server 2001 127.0.0.1:2181

server3:
	./$(BINDIR)/tree-server 2002 127.0.0.1:2181

client:
	./binary/tree-client 127.0.0.1:2181

grind-server1:
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose --log-file=valgrind-server1.txt ./$(BINDIR)/tree-server 9004 127.0.0.1:2181

grind-server2:
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose --log-file=valgrind-server2.txt ./$(BINDIR)/tree-server 9012 127.0.0.1:2181

grind-server3:
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose --log-file=valgrind-server3.txt ./$(BINDIR)/tree-server 9999 127.0.0.1:2181

# valgrind: test_data test_entry test_tree test_serialization
# 	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose --log-file=valgrind-data.txt ./binary/test_data
# 	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose --log-file=valgrind-entry.txt ./binary/test_entry
# 	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose --log-file=valgrind-tree.txt ./binary/test_tree
# 	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose --log-file=valgrind-serialization.txt ./binary/test_serialization^

# grind-server: tree-server
# 	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose --log-file=valgrind-server.txt ./binary/tree-server 12345

# grind-client: tree-client
# 	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose --log-file=valgrind-client.txt ./binary/tree-client 127.0.0.1:12345

