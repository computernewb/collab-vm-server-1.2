## Asio-VM

A library for interacting with virtual machines using Asio asynchronously and easily, in a
cross-platform manner.


## Abridged Example

This demo shows how to:

- Create a VM of a given type
  - In the case of QEMU, this effectively means "creating" a VM.

    For other hypervisors, we can use other means to setup VMs.
- Add a device to it (in this case storage)
- Start it, and await for it to be stopped

```cpp
#include <boost/asio.hpp>
#include <asio-vm/vm.hpp>

namespace asio = boost::asio;

int main() {
    boost::asio::io_service service;
    
    // this is a shared_ptr to the base VirtualMachine type.
    // it can be used if not using the single header to improve
    // compile time.
    vm::VirtualMachinePtr ptr = asio_vm::CreateVM(service,
            vm::VirtualMachineType::Qemu);
    
    // this can be used in implementation files
    // to access methods specific to a machine type.
    //
    // note that this method throws if you do something invalid
    auto qemuPtr = ptr->As<vm::qemu::QemuVirtualMachine>();
    
    // Add storage to the VM
    qemuPtr->AddStorage(vm::qemu::MakeStorage(
                // this will be the primary boot volume
                vm::StorageType::Disk,
                
                // This is an SSD for demonstration purposes.
                vm::StorageSubType::Ssd,
                
                // This can be left out for hot-pluggable media
                "./test.qcow2"
            ));
    
    // Spawn a coroutine to run our demo VM, and
    // wait for it to be stopped.
    // Once it stops the demo exits.
    asio::co_spawn(service, [newQemuPtr = qemuPtr]() {
        try {
            std::cout << "Starting demo VM\n";
            
            // Start the VM asynchronously, waiting for the start to succeed
            co_await newQemuPtr->AsyncStart(asio::use_awaitable);
            
            // WaitForStopEvent returns a QEMU specific type which can be used
            // to determine the stop cause.
            auto stopType = co_await newQemuPtr->WaitForStopEvent(asio::use_awaitable);
            
            std::cout << "Demo VM exited, exiting sample\n";
        } catch(const vm::exception& exception) {
            // an error occured.
        }
    });
    
    
    service.run();
    
    return 0;
}

```