#ifdef UNI_OMP
  #include <omp.h>
#endif
#include "Storage.hpp"
#include "utils/utils_internal_interface.hpp"
using namespace std;
using namespace cytnx;

namespace cytnx {
  //+++++++++++++++++++
  void FloatStorage::Init(const unsigned long long &len_in, const int &device,
                          const bool &init_zero) {
    // cout << "Float.init" << endl;
    // check:
    this->len = len_in;

    // check:
    // cytnx_error_msg(len_in < 1, "%s", "[ERROR] cannot init a Storage with zero element");
    this->dtype = Type.Float;

    if (this->len % STORAGE_DEFT_SZ) {
      this->cap = ((unsigned long long)((this->len) / STORAGE_DEFT_SZ) + 1) * STORAGE_DEFT_SZ;
    } else {
      this->cap = this->len;
    }

    if (device == Device.cpu) {
      if (init_zero)
        this->Mem = utils_internal::Calloc_cpu(this->cap, sizeof(float));
      else
        this->Mem = utils_internal::Malloc_cpu(this->cap * sizeof(float));
    } else {
#ifdef UNI_GPU
      cytnx_error_msg(device >= Device.Ngpus, "%s", "[ERROR] invalid device.");
      checkCudaErrors(cudaSetDevice(device));
      // this->Mem = utils_internal::cuMalloc_gpu(this->cap*sizeof(float));
      this->Mem = utils_internal::cuCalloc_gpu(this->cap, sizeof(float));
#else
      cytnx_error_msg(1, "%s", "[ERROR] cannot init a Storage on gpu without CUDA support.");
#endif
    }
    this->device = device;
  }

  void FloatStorage::_Init_byptr(void *rawptr, const unsigned long long &len_in, const int &device,
                                 const bool &iscap, const unsigned long long &cap_in) {
    this->Mem = rawptr;
    this->len = len_in;
    if (iscap) {
      this->cap = cap_in;
    } else {
      this->cap = len_in;
    }
    cytnx_error_msg(this->cap % STORAGE_DEFT_SZ != 0,
                    "[ERROR] _Init_by_ptr cannot have not %dx cap_in.", STORAGE_DEFT_SZ);

#ifdef UNI_DEBUG
    cytnx_error_msg(len_in < 1, "%s", "[ERROR] _Init_by_ptr cannot have len_in < 1.");
    cytnx_error_msg(this->cap < this->len, "%s",
                    "[ERROR] _Init_by_ptr cannot have capacity < size.");
#endif
    this->dtype = Type.Float;
    this->device = device;
  }

  boost::intrusive_ptr<Storage_base> FloatStorage::_create_new_sametype() {
    boost::intrusive_ptr<Storage_base> out(new FloatStorage());
    return out;
  }

  boost::intrusive_ptr<Storage_base> FloatStorage::clone() {
    boost::intrusive_ptr<Storage_base> out(new FloatStorage());
    out->Init(this->len, this->device);
    if (this->device == Device.cpu) {
      memcpy(out->Mem, this->Mem, sizeof(float) * this->len);
    } else {
#ifdef UNI_GPU
      checkCudaErrors(cudaSetDevice(this->device));
      checkCudaErrors(
        cudaMemcpy(out->Mem, this->Mem, sizeof(float) * this->len, cudaMemcpyDeviceToDevice));
#else
      cytnx_error_msg(1, "%s", "[ERROR] cannot clone a Storage on gpu without CUDA support.");
#endif
    }
    return out;
  }

  void FloatStorage::Move_memory_(const std::vector<cytnx_uint64> &old_shape,
                                  const std::vector<cytnx_uint64> &mapper,
                                  const std::vector<cytnx_uint64> &invmapper) {
    boost::intrusive_ptr<Storage_base> tmp(this);
    if (this->device == Device.cpu) {
      utils_internal::Movemem_cpu_f(tmp, old_shape, mapper, invmapper, 1);
    } else {
#ifdef UNI_GPU
      utils_internal::cuMovemem_gpu_f(tmp, old_shape, mapper, invmapper, 1);
#else
      cytnx_error_msg(1, "%s", "[ERROR][Internal] try to call GPU section without CUDA support");
#endif
    }
  }

  boost::intrusive_ptr<Storage_base> FloatStorage::Move_memory(
    const std::vector<cytnx_uint64> &old_shape, const std::vector<cytnx_uint64> &mapper,
    const std::vector<cytnx_uint64> &invmapper) {
    boost::intrusive_ptr<Storage_base> tmp(this);
    if (this->device == Device.cpu) {
      return utils_internal::Movemem_cpu_f(tmp, old_shape, mapper, invmapper, 0);
    } else {
#ifdef UNI_GPU
      return utils_internal::cuMovemem_gpu_f(tmp, old_shape, mapper, invmapper, 0);
#else
      cytnx_error_msg(1, "%s", "[ERROR][Internal] try to call GPU section without CUDA support");
      return nullptr;
#endif
    }
  }
  void FloatStorage::to_(const int &device) {
    if (this->device != device) {
      if (this->device == Device.cpu) {
// here, cpu->gpu with gid=device
#ifdef UNI_GPU
        cytnx_error_msg(device >= Device.Ngpus, "%s", "[ERROR] invalid device.");
        cudaSetDevice(device);
        void *dtmp = utils_internal::cuMalloc_gpu(sizeof(float) * this->cap);
        checkCudaErrors(
          cudaMemcpy(dtmp, this->Mem, sizeof(float) * this->len, cudaMemcpyHostToDevice));
        free(this->Mem);
        this->Mem = dtmp;
        this->device = device;
#else
        cytnx_error_msg(1, "%s", "[ERROR] try to move from cpu(Host) to gpu without CUDA support.");
#endif
      } else {
#ifdef UNI_GPU
        if (device == Device.cpu) {
          // here, gpu->cpu
          cudaSetDevice(this->device);
          void *htmp = malloc(sizeof(float) * this->cap);
          checkCudaErrors(
            cudaMemcpy(htmp, this->Mem, sizeof(float) * this->len, cudaMemcpyDeviceToHost));
          cudaFree(this->Mem);
          this->Mem = htmp;
          this->device = device;
        } else {
          // here, gpu->gpu
          cytnx_error_msg(device >= Device.Ngpus, "%s", "[ERROR] invalid device.");
          cudaSetDevice(device);
          void *dtmp = utils_internal::cuMalloc_gpu(sizeof(float) * this->cap);
          checkCudaErrors(
            cudaMemcpyPeer(dtmp, device, this->Mem, this->device, sizeof(float) * this->len));
          cudaFree(this->Mem);
          this->Mem = dtmp;
          this->device = device;
        }
#else
        cytnx_error_msg(
          1, "%s",
          "[ERROR][Internal] Storage.to_. the Storage is as GPU but without CUDA support.");
#endif
      }
    }
  }
  boost::intrusive_ptr<Storage_base> FloatStorage::to(const int &device) {
    // Here, we follow pytorch scheme. if the device is the same as this->device, then return this
    // (python self) otherwise, return a clone on different device.
    if (this->device == device) {
      return this;
    } else {
      if (this->device == Device.cpu) {
// here, cpu->gpu with gid=device
#ifdef UNI_GPU
        cytnx_error_msg(device >= Device.Ngpus, "%s", "[ERROR] invalid device.");
        cudaSetDevice(device);
        void *dtmp = utils_internal::cuMalloc_gpu(sizeof(float) * this->cap);
        checkCudaErrors(
          cudaMemcpy(dtmp, this->Mem, sizeof(float) * this->len, cudaMemcpyHostToDevice));
        boost::intrusive_ptr<Storage_base> out(new FloatStorage());
        out->_Init_byptr(dtmp, this->len, device, true, this->cap);
        return out;
#else
        cytnx_error_msg(1, "%s", "[ERROR] try to move from cpu(Host) to gpu without CUDA support.");
        return nullptr;
#endif
      } else {
#ifdef UNI_GPU
        if (device == Device.cpu) {
          // here, gpu->cpu
          cudaSetDevice(this->device);
          void *htmp = malloc(sizeof(float) * this->cap);
          checkCudaErrors(
            cudaMemcpy(htmp, this->Mem, sizeof(float) * this->len, cudaMemcpyDeviceToHost));
          boost::intrusive_ptr<Storage_base> out(new FloatStorage());
          out->_Init_byptr(htmp, this->len, device, true, this->cap);
          return out;
        } else {
          // here, gpu->gpu
          cytnx_error_msg(device >= Device.Ngpus, "%s", "[ERROR] invalid device.");
          cudaSetDevice(device);
          void *dtmp = utils_internal::cuMalloc_gpu(sizeof(float) * this->cap);
          checkCudaErrors(
            cudaMemcpyPeer(dtmp, device, this->Mem, this->device, sizeof(float) * this->len));
          boost::intrusive_ptr<Storage_base> out(new FloatStorage());
          out->_Init_byptr(dtmp, this->len, device, true, this->cap);
          return out;
        }
#else
        cytnx_error_msg(
          1, "%s",
          "[ERROR][Internal] Storage.to_. the Storage is as GPU but without CUDA support.");
        return nullptr;
#endif
      }
    }
  }

  void FloatStorage::PrintElem_byShape(std::ostream &os, const std::vector<cytnx_uint64> &shape,
                                       const std::vector<cytnx_uint64> &mapper) {
    char *buffer = (char *)malloc(sizeof(char) * 256);
    // checking:
    cytnx_uint64 Ne = 1;
    for (cytnx_uint64 i = 0; i < shape.size(); i++) {
      Ne *= shape[i];
    }
    if (Ne != this->len) {
      cytnx_error_msg(1, "%s",
                      "PrintElem_byShape, the number of shape not match with the No. of elements.");
    }

    if (len == 0) {
      os << "[ ";
      os << "\nThe Storage has not been allocated or linked.\n";
      os << "]\n";
    } else {
      os << std::endl << "Total elem: " << this->len << "\n";

      os << "type  : " << Type.getname(this->dtype) << std::endl;

      int atDevice = this->device;
      os << Device.getname(this->device) << std::endl;

      sprintf(buffer, "%s", "Shape :");
      os << std::string(buffer);
      sprintf(buffer, " (%d", shape[0]);
      os << std::string(buffer);
      for (cytnx_int32 i = 1; i < shape.size(); i++) {
        sprintf(buffer, ",%d", shape[i]);
        os << std::string(buffer);
      }
      os << ")" << std::endl;

      // temporary move to cpu for printing.
      if (this->device != Device.cpu) {
        this->to_(Device.cpu);
      }

      std::vector<cytnx_uint64> stk(shape.size(), 0), stk2;

      cytnx_uint64 s;
      cytnx_float *elem_ptr_ = static_cast<cytnx_float *>(this->Mem);

      if (mapper.size() == 0) {
        cytnx_uint64 cnt = 0;
        while (1) {
          for (cytnx_int32 i = 0; i < shape.size(); i++) {
            if (i < shape.size() - stk.size()) {
              sprintf(buffer, "%s", " ");
              os << std::string(buffer);
            } else {
              stk2.push_back(0);
              sprintf(buffer, "%s", "[");
              os << std::string(buffer);
              stk.pop_back();
            }
          }
          for (cytnx_uint64 i = 0; i < shape.back(); i++) {
            stk2.back() = i;
            sprintf(buffer, "%.5e ", elem_ptr_[cnt]);
            os << std::string(buffer);
            cnt++;
          }

          s = 0;
          while (1) {
            if (stk2.empty()) {
              break;
            }
            if (stk2.back() == *(&shape.back() - s) - 1) {
              stk.push_back(*(&shape.back() - s));
              s++;
              stk2.pop_back();
              sprintf(buffer, "%s", "]");
              os << std::string(buffer);
            } else {
              stk2.back() += 1;
              break;
            }
          }
          os << "\n";

          if (stk2.empty()) break;
        }
        os << std::endl;

      } else {
        /// This is for non-contiguous Tensor printing;
        // cytnx_error_msg(1,"%s","print for a non-contiguous Storage is under developing");
        // cytnx_uint64 cnt=0;
        std::vector<cytnx_uint64> c_offj(shape.size());
        std::vector<cytnx_uint64> c_shape(shape.size());

        cytnx_uint64 accu = 1;
        cytnx_uint64 RealMemPos;
        for (cytnx_uint32 i = 0; i < shape.size(); i++) {
          c_shape[i] = shape[mapper[i]];
        }
        for (cytnx_int64 i = c_shape.size() - 1; i >= 0; i--) {
          c_offj[i] = accu;
          accu *= c_shape[i];
        }

        while (1) {
          for (cytnx_int32 i = 0; i < shape.size(); i++) {
            if (i < shape.size() - stk.size()) {
              sprintf(buffer, "%s", " ");
              os << std::string(buffer);
            } else {
              stk2.push_back(0);
              sprintf(buffer, "%s", "[");
              os << std::string(buffer);
              stk.pop_back();
            }
          }
          for (cytnx_uint64 i = 0; i < shape.back(); i++) {
            stk2.back() = i;

            /// Calculate the Memory reflection:
            RealMemPos = 0;
            for (cytnx_uint64 n = 0; n < shape.size(); n++) {
              RealMemPos += c_offj[n] * stk2[mapper[n]];  // mapback + backmap = normal-map
            }
            sprintf(buffer, "%.5e ", elem_ptr_[RealMemPos]);
            os << std::string(buffer);
            // cnt++;
          }

          s = 0;
          while (1) {
            if (stk2.empty()) {
              break;
            }
            if (stk2.back() == *(&shape.back() - s) - 1) {
              stk.push_back(*(&shape.back() - s));
              s++;
              stk2.pop_back();
              sprintf(buffer, "%s", "]");
              os << std::string(buffer);
            } else {
              stk2.back() += 1;
              break;
            }
          }
          os << "\n";

          if (stk2.empty()) break;
        }
        os << std::endl;

      }  // check if need mapping

      if (atDevice != Device.cpu) {
        this->to_(atDevice);
      }

    }  // len==0
    free(buffer);
  }

  void FloatStorage::print_elems() {
    char *buffer = (char *)malloc(sizeof(char) * 256);
    cytnx_float *elem_ptr_ = static_cast<cytnx_float *>(this->Mem);
    cout << "[ ";
    for (unsigned long long cnt = 0; cnt < this->len; cnt++) {
      sprintf(buffer, "%.5e ", elem_ptr_[cnt]);
      cout << std::string(buffer);
    }
    cout << "]" << endl;
    free(buffer);
  }

  void FloatStorage::fill(const cytnx_complex128 &val) {
    cytnx_error_msg(true, "[ERROR]%s", " cannot fill complex value into real container");
  }
  void FloatStorage::fill(const cytnx_complex64 &val) {
    cytnx_error_msg(true, "[ERROR]%s", " cannot fill complex value into real container");
  }
  void FloatStorage::fill(const cytnx_double &val) {
    cytnx_float tmp = val;
    if (this->device == Device.cpu) {
      utils_internal::Fill_cpu_f(this->Mem, (void *)(&tmp), this->len);
    } else {
#ifdef UNI_GPU
      utils_internal::cuFill_gpu_f(this->Mem, (void *)(&tmp), this->len);
#else
      cytnx_error_msg(true, "[ERROR][fill] fatal internal, %s",
                      "storage is on gpu without CUDA support\n");
#endif
    }
  }
  void FloatStorage::fill(const cytnx_float &val) {
    if (this->device == Device.cpu) {
      utils_internal::Fill_cpu_f(this->Mem, (void *)(&val), this->len);
    } else {
#ifdef UNI_GPU
      utils_internal::cuFill_gpu_f(this->Mem, (void *)(&val), this->len);
#else
      cytnx_error_msg(true, "[ERROR][fill] fatal internal, %s",
                      "storage is on gpu without CUDA support\n");
#endif
    }
  }
  void FloatStorage::fill(const cytnx_int64 &val) {
    cytnx_float tmp = val;
    if (this->device == Device.cpu) {
      utils_internal::Fill_cpu_f(this->Mem, (void *)(&tmp), this->len);
    } else {
#ifdef UNI_GPU
      utils_internal::cuFill_gpu_f(this->Mem, (void *)(&tmp), this->len);
#else
      cytnx_error_msg(true, "[ERROR][fill] fatal internal, %s",
                      "storage is on gpu without CUDA support\n");
#endif
    }
  }
  void FloatStorage::fill(const cytnx_uint64 &val) {
    cytnx_float tmp = val;
    if (this->device == Device.cpu) {
      utils_internal::Fill_cpu_f(this->Mem, (void *)(&tmp), this->len);
    } else {
#ifdef UNI_GPU
      utils_internal::cuFill_gpu_f(this->Mem, (void *)(&tmp), this->len);
#else
      cytnx_error_msg(true, "[ERROR][fill] fatal internal, %s",
                      "storage is on gpu without CUDA support\n");
#endif
    }
  }
  void FloatStorage::fill(const cytnx_int32 &val) {
    cytnx_float tmp = val;
    if (this->device == Device.cpu) {
      utils_internal::Fill_cpu_f(this->Mem, (void *)(&tmp), this->len);
    } else {
#ifdef UNI_GPU
      utils_internal::cuFill_gpu_f(this->Mem, (void *)(&tmp), this->len);
#else
      cytnx_error_msg(true, "[ERROR][fill] fatal internal, %s",
                      "storage is on gpu without CUDA support\n");
#endif
    }
  }
  void FloatStorage::fill(const cytnx_uint32 &val) {
    cytnx_float tmp = val;
    if (this->device == Device.cpu) {
      utils_internal::Fill_cpu_f(this->Mem, (void *)(&tmp), this->len);
    } else {
#ifdef UNI_GPU
      utils_internal::cuFill_gpu_f(this->Mem, (void *)(&tmp), this->len);
#else
      cytnx_error_msg(true, "[ERROR][fill] fatal internal, %s",
                      "storage is on gpu without CUDA support\n");
#endif
    }
  }
  void FloatStorage::fill(const cytnx_int16 &val) {
    cytnx_float tmp = val;
    if (this->device == Device.cpu) {
      utils_internal::Fill_cpu_f(this->Mem, (void *)(&tmp), this->len);
    } else {
#ifdef UNI_GPU
      utils_internal::cuFill_gpu_f(this->Mem, (void *)(&tmp), this->len);
#else
      cytnx_error_msg(true, "[ERROR][fill] fatal internal, %s",
                      "storage is on gpu without CUDA support\n");
#endif
    }
  }
  void FloatStorage::fill(const cytnx_uint16 &val) {
    cytnx_float tmp = val;
    if (this->device == Device.cpu) {
      utils_internal::Fill_cpu_f(this->Mem, (void *)(&tmp), this->len);
    } else {
#ifdef UNI_GPU
      utils_internal::cuFill_gpu_f(this->Mem, (void *)(&tmp), this->len);
#else
      cytnx_error_msg(true, "[ERROR][fill] fatal internal, %s",
                      "storage is on gpu without CUDA support\n");
#endif
    }
  }
  void FloatStorage::fill(const cytnx_bool &val) {
    cytnx_float tmp = val;
    if (this->device == Device.cpu) {
      utils_internal::Fill_cpu_f(this->Mem, (void *)(&tmp), this->len);
    } else {
#ifdef UNI_GPU
      utils_internal::cuFill_gpu_f(this->Mem, (void *)(&tmp), this->len);
#else
      cytnx_error_msg(true, "[ERROR][fill] fatal internal, %s",
                      "storage is on gpu without CUDA support\n");
#endif
    }
  }

  void FloatStorage::set_zeros() {
    if (this->device == Device.cpu) {
      utils_internal::SetZeros(this->Mem, sizeof(cytnx_float) * this->len);
    } else {
#ifdef UNI_GPU
      utils_internal::cuSetZeros(this->Mem, sizeof(cytnx_float) * this->len);
#else
      cytnx_error_msg(1, "[ERROR][set_zeros] fatal, the storage is on gpu without CUDA support.%s",
                      "\n");
#endif
    }
  }

  void FloatStorage::resize(const cytnx_uint64 &newsize) {
    // cytnx_error_msg(newsize < 1,"[ERROR]resize should have size > 0%s","\n");

    if (newsize > this->cap) {
      if (newsize % STORAGE_DEFT_SZ) {
        this->cap = ((unsigned long long)((newsize) / STORAGE_DEFT_SZ) + 1) * STORAGE_DEFT_SZ;
      } else {
        this->cap = newsize;
      }
      if (this->device == Device.cpu) {
        void *htmp = calloc(this->cap, sizeof(cytnx_float));
        memcpy(htmp, this->Mem, sizeof(cytnx_float) * this->len);
        free(this->Mem);
        this->Mem = htmp;
      } else {
#ifdef UNI_GPU
        cytnx_error_msg(device >= Device.Ngpus, "%s", "[ERROR] invalid device.");
        cudaSetDevice(device);
        void *dtmp = utils_internal::cuCalloc_gpu(this->cap, sizeof(cytnx_float));
        checkCudaErrors(
          cudaMemcpyPeer(dtmp, device, this->Mem, this->device, sizeof(float) * this->len));
        cudaFree(this->Mem);
        this->Mem = dtmp;
#else
        cytnx_error_msg(
          1, "%s",
          "[ERROR][Internal] Storage.resize. the Storage is as GPU but without CUDA support.");
#endif
      }
    }
    this->len = newsize;
  }

  void FloatStorage::append(const cytnx_complex128 &val) {
    cytnx_error_msg(true, "[ERROR]%s", " cannot append complex value into real container");
  }
  void FloatStorage::append(const cytnx_complex64 &val) {
    cytnx_error_msg(true, "[ERROR]%s", " cannot append complex value into real container");
  }

  void FloatStorage::append(const Scalar &val) {
    if (this->len + 1 > this->cap) {
      this->resize(this->len + 1);
    } else {
      this->len += 1;
    }
    this->at<cytnx_float>(this->len - 1) = float(val);
  }
  void FloatStorage::append(const cytnx_double &val) {
    if (this->len + 1 > this->cap) {
      this->resize(this->len + 1);
    } else {
      this->len += 1;
    }
    this->at<cytnx_float>(this->len - 1) = val;
  }
  void FloatStorage::append(const cytnx_float &val) {
    if (this->len + 1 > this->cap) {
      this->resize(this->len + 1);
    } else {
      this->len += 1;
    }
    this->at<cytnx_float>(this->len - 1) = val;
  }
  void FloatStorage::append(const cytnx_int64 &val) {
    if (this->len + 1 > this->cap) {
      this->resize(this->len + 1);
    } else {
      this->len += 1;
    }
    this->at<cytnx_float>(this->len - 1) = val;
  }
  void FloatStorage::append(const cytnx_int32 &val) {
    if (this->len + 1 > this->cap) {
      this->resize(this->len + 1);
    } else {
      this->len += 1;
    }
    this->at<cytnx_float>(this->len - 1) = val;
  }
  void FloatStorage::append(const cytnx_int16 &val) {
    if (this->len + 1 > this->cap) {
      this->resize(this->len + 1);
    } else {
      this->len += 1;
    }
    this->at<cytnx_float>(this->len - 1) = val;
  }
  void FloatStorage::append(const cytnx_uint64 &val) {
    if (this->len + 1 > this->cap) {
      this->resize(this->len + 1);
    } else {
      this->len += 1;
    }
    this->at<cytnx_float>(this->len - 1) = val;
  }
  void FloatStorage::append(const cytnx_uint32 &val) {
    if (this->len + 1 > this->cap) {
      this->resize(this->len + 1);
    } else {
      this->len += 1;
    }
    this->at<cytnx_float>(this->len - 1) = val;
  }
  void FloatStorage::append(const cytnx_uint16 &val) {
    if (this->len + 1 > this->cap) {
      this->resize(this->len + 1);
    } else {
      this->len += 1;
    }
    this->at<cytnx_float>(this->len - 1) = val;
  }
  void FloatStorage::append(const cytnx_bool &val) {
    if (this->len + 1 > this->cap) {
      this->resize(this->len + 1);
    } else {
      this->len += 1;
    }
    this->at<cytnx_float>(this->len - 1) = val;
  }

  boost::intrusive_ptr<Storage_base> FloatStorage::real() {
    cytnx_error_msg(true, "[ERROR] Storage.real() can only be called from complex type.%s", "\n");
  }
  boost::intrusive_ptr<Storage_base> FloatStorage::imag() {
    cytnx_error_msg(true, "[ERROR] Storage.imag() can only be called from complex type.%s", "\n");
  }
  Scalar FloatStorage::get_item(const cytnx_uint64 &idx) const {
    return Scalar(this->at<cytnx_float>(idx));
  }

  void FloatStorage::set_item(const cytnx_uint64 &idx, const Scalar &val) {
    this->at<cytnx_float>(idx) = cytnx_float(val);
  }
  void FloatStorage::set_item(const cytnx_uint64 &idx, const cytnx_complex128 &val) {
    cytnx_error_msg(true, "[ERROR] cannot set complex to real.%s", "\n");
  }
  void FloatStorage::set_item(const cytnx_uint64 &idx, const cytnx_complex64 &val) {
    cytnx_error_msg(true, "[ERROR] cannot set complex to real.%s", "\n");
  }
  void FloatStorage::set_item(const cytnx_uint64 &idx, const cytnx_double &val) {
    this->at<cytnx_float>(idx) = val;
  }
  void FloatStorage::set_item(const cytnx_uint64 &idx, const cytnx_float &val) {
    this->at<cytnx_float>(idx) = val;
  }
  void FloatStorage::set_item(const cytnx_uint64 &idx, const cytnx_int64 &val) {
    this->at<cytnx_float>(idx) = val;
  }
  void FloatStorage::set_item(const cytnx_uint64 &idx, const cytnx_uint64 &val) {
    this->at<cytnx_float>(idx) = val;
  }
  void FloatStorage::set_item(const cytnx_uint64 &idx, const cytnx_int32 &val) {
    this->at<cytnx_float>(idx) = val;
  }
  void FloatStorage::set_item(const cytnx_uint64 &idx, const cytnx_uint32 &val) {
    this->at<cytnx_float>(idx) = val;
  }
  void FloatStorage::set_item(const cytnx_uint64 &idx, const cytnx_int16 &val) {
    this->at<cytnx_float>(idx) = val;
  }
  void FloatStorage::set_item(const cytnx_uint64 &idx, const cytnx_uint16 &val) {
    this->at<cytnx_float>(idx) = val;
  }
  void FloatStorage::set_item(const cytnx_uint64 &idx, const cytnx_bool &val) {
    this->at<cytnx_float>(idx) = val;
  }

  bool FloatStorage::equivelem(const boost::intrusive_ptr<Storage_base> &rhs,
                               const cytnx_double tol) {
    if (rhs->dtype != Type.Float) return false;
    if (rhs->size() != this->len) return false;
    for (cytnx_uint64 i = 0; i < this->len; i++) {
      if (abs(this->at<cytnx_float>(i) - rhs->at<cytnx_float>(i)) > tol) {
        std::cout << "tensor different at idx:" << i << "\nlhs:" << this->at<cytnx_float>(i)
                  << " rhs:" << rhs->at<cytnx_float>(i) << "\n"
                  << "difference in absolute value: "
                  << abs(this->at<cytnx_float>(i) - rhs->at<cytnx_float>(i)) << std::endl;
        return false;
      }
    }
    return true;
  }
}  // namespace cytnx
